#define STB_TRUETYPE_IMPLEMENTATION
#include "ResFontFT.h"

#include "oxygine/Font.h"
#include "oxygine/Image.h"
#include "oxygine/core/ImageDataOperations.h"
#include "oxygine/core/NativeTexture.h"
#include "oxygine/core/VideoDriver.h"
#include "oxygine/res/CreateResourceContext.h"
#include "oxygine/res/Resources.h"
#include "oxygine/utils/stringUtils.h"

namespace oxygine {
int utfByteLength(uint8_t nextByte) {
   if ((nextByte & 0x80) == 0)
      return 1;
   else if ((nextByte & 0xC0) == 0x80)
      return -1;
   else if ((nextByte & 0xE0) == 0xC0)
      return 2;
   else if ((nextByte & 0xF0) == 0xE0)
      return 3;
   else if ((nextByte & 0xF8) == 0xF0)
      return 4;
   else
      return 0;
}
uint8_t utfBitMaskSize(uint8_t nextByte) {
   if ((nextByte & 0x80) == 0)
      return 7;
   else if ((nextByte & 0xC0) == 0x80)
      return 6;
   else if ((nextByte & 0xE0) == 0xC0)
      return 5;
   else if ((nextByte & 0xF0) == 0xE0)
      return 4;
   else if ((nextByte & 0xF8) == 0xF0)
      return 3;
   else
      return 0;
}
uint32_t decodeSymbol(int utf8) {
   uint8_t* bytes = (uint8_t*)&utf8;
   uint32_t rval = 0;

   int size = utfByteLength(bytes[0]);
   for (int b = 0; b < size; ++b) {
      uint8_t msz = utfBitMaskSize(bytes[b]);
      uint8_t mask = 0xFF >> (8 - msz);
      uint8_t data = bytes[b] & mask;
      uint8_t shift = msz;
      if (b == 0) shift = 0;
      rval = (rval << shift) | data;
   }
   return rval;
}

int encodeSymbol(uint32_t unic) {
   int rval = 0;
   unsigned char* utf8 = (unsigned char*)&rval;

   if (unic < 0x80) {
      utf8[0] = (unic >> 0 & 0x7F) | 0x00;
   } else if (unic < 0x0800) {
      utf8[0] = (unic >> 6 & 0x1F) | 0xC0;
      utf8[1] = (unic >> 0 & 0x3F) | 0x80;
   } else if (unic < 0x010000) {
      utf8[0] = (unic >> 12 & 0x0F) | 0xE0;
      utf8[1] = (unic >> 6 & 0x3F) | 0x80;
      utf8[2] = (unic >> 0 & 0x3F) | 0x80;
   } else {
      utf8[0] = (unic >> 18 & 0x07) | 0xF0;
      utf8[1] = (unic >> 12 & 0x3F) | 0x80;
      utf8[2] = (unic >> 6 & 0x3F) | 0x80;
      utf8[3] = (unic >> 0 & 0x3F) | 0x80;
   }

   return rval;
}

static oxygine::Point FT_ATLAS_SIZE(512, 512);

void ftGenDefault(ResFontFT::postProcessData& data) {
   Image& dest = *data.dest;
   const ImageData& src = *data.src;

   dest.init(src.w, src.h, TF_R8G8B8A8);
   ImageData rc = dest.lock();
   operations::blitPremultiply(src, rc);
}

static ResFontFT::postProcessHook _ftGen = ftGenDefault;

void ResFontFT::setGlyphPostProcessor(postProcessHook f) {
   _ftGen = f;
}

Image tempImage;
#define STBTT_SCALE 1.2f
#define SBTT_SDF_SIZE 20
class FontFT : public Font {
  public:
   FontFT(ResFontFT* rs, int size, bool SDF) : _rs(rs), _size(size) {
      OX_ASSERT(size > 0);

      if (size <= 0) size = 10;
      _ignoreOptions = true;

      stbtt_fontinfo& face = *_rs->_faces.begin();
      int ascent;
      int descent;
      int linegap;
      stbtt_GetFontVMetrics(&face, &ascent, &descent, &linegap);
      float scale = stbtt_ScaleForPixelHeight(&face, _size * STBTT_SCALE);

      int baseline = ascent * scale;
      int mxadv = (ascent - descent + linegap) * scale;
      _SDF = SDF;

      init("TTF Font", size, baseline, mxadv, SDF);
      logs::messageln("FONT %p=>%p::%d %f", rs, this, size, scale);
   }
   virtual int getPadding() const override { return SBTT_SDF_SIZE / 2; }
   virtual bool BiDiPass(std::vector<text::Symbol*>& line) const override {
      if (_rs->bidiDelegate())
         return _rs->bidiDelegate()(line);
      else
         return false;
   }

  protected:
   ResFontFT* _rs;
   int _size;
   bool _SDF;
   bool loadGlyph(int code, glyph& g, const glyphOptions& opt) override {
      bool found = false;
      stbtt_fontinfo* face;
      int index = 0;
      uint32_t unicode = decodeSymbol(code);

      for (auto it = _rs->_faces.begin(); it != _rs->_faces.end(); ++it) {
         face = &(*it);

         /* load glyph image into the slot (erase previous one) */
         index = stbtt_FindGlyphIndex(face, unicode);

         if (index != 0) {
            found = true;
            break;
         }
      }

      if (!found) {
         if (_rs->notFoundCB) _rs->notFoundCB(unicode);
         return false;
      }
      Point g_size(0);
      Point g_off(0);

      int oneside = 128;
      int padding = getPadding();

      float scale = stbtt_ScaleForPixelHeight(face, _size * STBTT_SCALE);
      uint8_t* bitmap;
      if (_SDF)
         bitmap = stbtt_GetGlyphSDF(face, scale, index, padding, oneside, (uint8_t)(oneside / padding), &g_size.x, &g_size.y, &g_off.x, &g_off.y);
      else {
         bitmap = stbtt_GetGlyphBitmap(face, scale, scale, index, &g_size.x, &g_size.y, &g_off.x, &g_off.y);
         padding = 0;
      }
      int advance;
      int bearing;
      stbtt_GetGlyphHMetrics(face, index, &advance, &bearing);

      ImageData src(g_size.x, g_size.y, g_size.x, TF_A8, bitmap);

      Rect srcRect;
      spTexture t;

      g.advance_x = advance * scale;
      g.advance_y = 0;
      g.offset_x = g_off.x - 1;
      g.offset_y = g_off.y - 1;
      g.ch = code;
      g.opt = 0;  // opt;

      // if (src.w && src.h)
      {
         ResFontFT::postProcessData gd;
         gd.src = &src;
         gd.dest = &tempImage;
         gd.gl = &g;
         gd.opt = opt;
         gd.font = this;

         if (src.w && src.h)
            _ftGen(gd);
         else {
            tempImage.init(0, 0, TF_R8G8B8A8);
         }

         _rs->_atlas.add(tempImage.lock(), srcRect, t);
         OX_ASSERT(t);
         g.src = srcRect.cast<RectF>();
         Vector2 sz((float)t->getWidth(), (float)t->getHeight());
         g.src.pos = g.src.pos / sz;
         g.src.size = g.src.size / sz;
         g.texture = safeSpCast<NativeTexture>(t);
      }

      g.sw = tempImage.getWidth();
      g.sh = tempImage.getHeight();

      /*
      std::string utf8;
      oxygine::charCode2Bytes(utf8, code);
      logs::messageln("GLYPH <%s> [%d] [%d,%d]x[%d,%d]x[%d,%d]", utf8.c_str(), padding, g.offset_x, g.offset_y, g.advance_x, g.advance_y, g.sw, g.sh);
      */
      stbtt_FreeBitmap(bitmap, nullptr);

      return true;
   }
};

Resource* ResFontFT::createResource(CreateResourceContext& context) {
   ResFontFT* res = new ResFontFT;

   pugi::xml_node node = context.walker.getNode();

   setNode(res, node);
   std::string file = context.walker.getPath("file");
   res->setName(Resource::extractID(node, file, ""));
   res->init(file);
   context.resources->add(res);
   return res;
}

void ResFontFT::initLibrary() {
   Resources::registerResourceType(&ResFontFT::createResource, "ftfont");
}

void ResFontFT::freeLibrary() {
   Resources::unregisterResourceType("ftfont");
}

void ResFontFT::setAtlasSize(int w, int h) {
   FT_ATLAS_SIZE = Point(w, h);
}

ResFontFT::ResFontFT() : _atlas(CLOSURE(this, &ResFontFT::createTexture)), _bidiDelegate(NULL) {
   _atlas.init();
}

ResFontFT::~ResFontFT() {}

spTexture ResFontFT::createTexture(int w, int h) {
   Image mt;

   mt.init(FT_ATLAS_SIZE.x, FT_ATLAS_SIZE.y, TF_R8G8B8A8);
   mt.fillZero();

   spNativeTexture texture = IVideoDriver::instance->createTexture();
   texture->init(mt.lock());

   return texture;
}

void ResFontFT::init(const std::string& fnt) {
   _file = fnt;
}

Font* ResFontFT::getFont(int size) {
   OX_ASSERT(size >= 0);

   if (size <= 0) size = 10;

   for (fonts::iterator i = _fonts.begin(); i != _fonts.end(); ++i) {
      FontFT* f = &(*i);

      if (f->getSize() == size) return f;
   }
   _fonts.push_back(FontFT(this, size, true));
   return &_fonts.back();
}

const Font* ResFontFT::getFont(const char* name, int size) const {
   // OX_ASSERT(size > 0);
   ResFontFT* r = const_cast<ResFontFT*>(this);

   return r->getFont(size);
}

const oxygine::Font* ResFontFT::getClosestFont(float worldScale, int styleFontSize, float& resScale) const {
   if (!styleFontSize) return 0;

   int fontSize = styleFontSize;

   int delta = fontSize % SBTT_SDF_SIZE;

   if (delta > SBTT_SDF_SIZE / 2)
      fontSize += SBTT_SDF_SIZE - delta;
   else {
      fontSize -= delta;
      if (fontSize < SBTT_SDF_SIZE) fontSize = SBTT_SDF_SIZE;
   }

   resScale = (float)fontSize / (float)styleFontSize;
   return getFont(0, fontSize);
}

void ResFontFT::_load(LoadResourcesContext* context) {
   if (!_file.empty()) {
      file::read(_file.c_str(), _fdata);

      stbtt_fontinfo _face;
      int success = stbtt_InitFont(&_face, (const uint8_t*)_fdata.getData(), 0);
      _faces.push_back(_face);
      OX_ASSERT(success);
      _file = "";
   }
}

void ResFontFT::addFace(const unsigned char* data, size_t size) {
   stbtt_fontinfo _face;
   int success = stbtt_InitFont(&_face, data, 0);

   if (success) {
      _faces.push_back(_face);

      for (auto it = _fonts.begin(); it != _fonts.end(); ++it) {
         it->rehash();
      }
   }
}

void ResFontFT::setNotFoundCallback(symbolCallback cb) {
   this->notFoundCB = cb;

   for (auto it = _fonts.begin(); it != _fonts.end(); ++it) {
      it->rehash();
   }
}

void ResFontFT::_unload() {
   _faces.clear();
}

ResFontFT::BiDiCallback ResFontFT::bidiDelegate() {
   return _bidiDelegate;
}
}  // namespace oxygine
