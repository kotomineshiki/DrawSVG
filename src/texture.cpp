#include "texture.h"
#include "color.h"

#include <assert.h>
#include <iostream>
#include <algorithm>

using namespace std;

namespace CMU462 {

inline void uint8_to_float( float dst[4], unsigned char* src ) {
  uint8_t* src_uint8 = (uint8_t *)src;
  dst[0] = src_uint8[0] / 255.f;
  dst[1] = src_uint8[1] / 255.f;
  dst[2] = src_uint8[2] / 255.f;
  dst[3] = src_uint8[3] / 255.f;
}

inline void float_to_uint8( unsigned char* dst, float src[4] ) {
  uint8_t* dst_uint8 = (uint8_t *)dst;
  dst_uint8[0] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[0])));
  dst_uint8[1] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[1])));
  dst_uint8[2] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[2])));
  dst_uint8[3] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[3])));
}

void Sampler2DImp::generate_mips(Texture& tex, int startLevel) {

  // NOTE: 
  // This starter code allocates the mip levels and generates a level 
  // map by filling each level with placeholder data in the form of a 
  // color that differs from its neighbours'. You should instead fill
  // with the correct data!

  // Task 7: Implement this

  // check start level
  if ( startLevel >= tex.mipmap.size() ) {
    std::cerr << "Invalid start level"; 
  }

  // allocate sublevels
  int baseWidth  = tex.mipmap[startLevel].width;
  int baseHeight = tex.mipmap[startLevel].height;
  int numSubLevels = (int)(log2f( (float)max(baseWidth, baseHeight)));

  numSubLevels = min(numSubLevels, kMaxMipLevels - startLevel - 1);
  tex.mipmap.resize(startLevel + numSubLevels + 1);

  int width  = baseWidth;
  int height = baseHeight;
  for (int i = 1; i <= numSubLevels; i++) {

    MipLevel& level = tex.mipmap[startLevel + i];

    // handle odd size texture by rounding down
    width  = max( 1, width  / 2); assert(width  > 0);
    height = max( 1, height / 2); assert(height > 0);

    level.width = width;
    level.height = height;
    level.texels = vector<unsigned char>(4 * width * height);

  }
  cout <<"GenerateMipmapSize:" << tex.mipmap.size()<<endl;
  // fill all 0 sub levels with interchanging colors (JUST AS A PLACEHOLDER)
  Color colors[3] = { Color(1,0,0,1), Color(0,1,0,1), Color(0,0,1,1) };
  for (size_t i = 1; i < tex.mipmap.size(); ++i) {

      //  Color c = colors[i % 3];
      MipLevel& mip = tex.mipmap[i];
      MipLevel& last_mip = tex.mipmap[i - 1];
      for (int m = 0;m < mip.width;++m) {
          for (int n = 0;n < mip.height;++n) {
              int base = 4 * (n * mip.width + m);
              Color c = (last_mip.color(2 * m, 2 * n) + last_mip.color(2 * m + 1, 2 * n) + last_mip.color(2 * m, 2 * n + 1) + last_mip.color(2 * m + 1, 2 * n + 1)) * 0.25;
              mip.texels[base] = c.r * 255;
              mip.texels[base + 1] = c.g * 255;
              mip.texels[base + 2] = c.b * 255;
              mip.texels[base + 3] = 1 * 255;
          }
      }
  }

}

Color Sampler2DImp::sample_nearest(Texture& tex, 
                                   float u, float v, 
                                   int level) {

  // Task 6: Implement nearest neighbour interpolation
    int tu = round(u * float(tex.width));
    int tv = round(v * float(tex.height));
  // return magenta for invalid level
    if (0 > tu || tu > tex.width || 0 > tv || tv > tex.height)
        return Color(1, 0, 1, 1);
    //cout << "Samled";
    size_t base = 4 * (tu * tex.width + tv);
    //return Color(0.5, 0.5, 0.5, 1);
    return Color(
        tex.mipmap[0].texels[base] / 255.0f,
        tex.mipmap[0].texels[base + 1] / 255.0f,
        tex.mipmap[0].texels[base + 2] / 255.0f,
        tex.mipmap[0].texels[base + 3] / 255.0f
    );

}

Color Sampler2DImp::sample_bilinear(Texture& tex, 
                                    float u, float v, 
                                    int level) {
  
  // Task 6: Implement bilinear filtering
    MipLevel mipmap= tex.mipmap[level];
    u *= float(mipmap.width);
    v *= float(mipmap.height);
    if (0 > u || u > tex.width || 0 > v || v > tex.height)
        return Color(1, 0, 1, 1);
    int fu = floor(u-0.5);
    int fv = floor(v-0.5);
    int cu = fu + 1;
    int cv = fv + 1;

    Color color1 = mipmap.valid(fu,fv)? mipmap.color(fu,fv):Color::Black;
    Color color2 = mipmap.valid(fu, cv) ? mipmap.color(fu, cv) : Color::Black;
    Color color3= mipmap.valid(cu, fv) ? mipmap.color(cu, fv) : Color::Black;
    Color color4 = mipmap.valid(cu, cv) ? mipmap.color(cu, cv) : Color::Black;
    float s = (u - (fu+0.5));
    float t = (v - (fv+0.5));
    Color linearColor1 = color1 * (1 - s) + color3 * s;
    Color linearColor2 = color2 * (1 - s) + color4 * s;
    Color bilinearColor = (1 - t) * linearColor1 + t * linearColor2;
    return bilinearColor;
    //return Color(0.5, 0.5, 0.5, 1);

}

Color Sampler2DImp::sample_trilinear(Texture& tex,
    float u, float v,
    float u_scale, float v_scale) {

    // Task 7: Implement trilinear filtering
    float level = -log2(u_scale);

    // return magenta for invalid level
    if (level <= 0)return sample_bilinear(tex, u, v, 0);
    if (level >= tex.mipmap.size())return sample_bilinear(tex, u, v, tex.mipmap.size() - 1);
    int current_level = floor(level);
    int next_level = current_level + 1;
    float r = level - current_level;
    Color current_color = sample_bilinear(tex, u, v, current_level);
    Color next_color = sample_bilinear(tex, u, v, next_level);
    Color trilinear_color = current_color * (1 - r) + next_color * r;
    return trilinear_color;
}

} // namespace CMU462
