#include "ddsloader.h"
#include "ddsfile.h"

#include <SFML/OpenGL.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <ostream>
#include <string>

using dword = unsigned int;

const dword DDS_MAGIC = 0x20534444;
const dword DDPF_ALPHAPIXELS = 0x1;
const dword DDPF_FOURCC = 0x4;
const dword DDPF_RGB = 0x40;

constexpr dword fourCcToDword(const char *value) {
  return value[0] + (value[1] << 8) + (value[2] << 16) + (value[3] << 24);
}

constexpr dword DXT1 = fourCcToDword("DXT1");
constexpr dword DXT2 = fourCcToDword("DXT2");
constexpr dword DXT3 = fourCcToDword("DXT3");
constexpr dword DXT4 = fourCcToDword("DXT4");
constexpr dword DXT5 = fourCcToDword("DXT5");
constexpr dword DX10 = fourCcToDword("DX10");

struct DdsPixelFormat {
  dword dwSize;
  dword dwFlags;
  dword dwFourCC;
  dword dwRGBBitCount;
  dword dwRBitMask;
  dword dwGBitMask;
  dword dwBBitMask;
  dword dwABitMask;
};

struct DdsHeader {
  dword dwSize;
  dword dwFlags;
  dword dwHeight;
  dword dwWidth;
  dword dwPitchOrLinearSize;
  dword dwDepth;
  dword dwMipMapCount;
  dword dwReserved1[11];
  DdsPixelFormat ddspf;
  dword dwCaps;
  dword dwCaps2;
  dword dwCaps3;
  dword dwCaps4;
  dword dwReserved2;
};

struct DdsHeaderDxt10 {
  int dxgiFormat;
  int resourceDimension;
  unsigned int miscFlag;
  unsigned int arraySize;
  unsigned int miscFlags2;
};

template <typename Struct> void read(std::ifstream &file, Struct &output) {
  file.read(reinterpret_cast<char *>(&output), sizeof(Struct));
}

std::unique_ptr<DdsFile> DdsLoader::load(const std::string &filePath) {
  std::ifstream file{filePath};

  if (file.is_open()) {
    auto result = std::unique_ptr<DdsFile>(new DdsFile{filePath});
    std::vector<char> data;
    DdsHeader header;
    dword magic;

    read(file, magic);

    if (magic != DDS_MAGIC) {
      std::cout << "Wrong magic number, not a dds file?" << std::endl;
      return nullptr;
    }

    read(file, header);

    std::cout << "Dimensions: " << header.dwWidth << "x" << header.dwHeight
              << std::endl;
    std::cout << "Mipmap count:" << header.dwMipMapCount << std::endl;
    std::cout << "Flags: " << header.ddspf.dwFlags << std::endl;

    int width = header.dwWidth;
    int height = header.dwHeight;

    if (header.ddspf.dwFlags & DDPF_FOURCC) {
      int blocSize;
      GLenum glFormat;

      switch (header.ddspf.dwFourCC) {
      case DXT1: {
        blocSize = 8;
        glFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
      } break;

      case DXT2: {
        blocSize = 16;
      } break;

      case DXT3: {
        blocSize = 16;
        glFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
      } break;

      case DXT4: {
        blocSize = 16;
      } break;

      case DXT5: {
        blocSize = 16;
        glFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
      } break;

      case DX10: {
        DdsHeaderDxt10 headerDxt10;
        read(file, headerDxt10);
        std::cout << "DX10" << std::endl;
        std::cout << "Format: " << headerDxt10.dxgiFormat << std::endl;
        std::cout << "Resource dimension: " << headerDxt10.resourceDimension
                  << std::endl;
        std::cout << "Misc flag: " << headerDxt10.miscFlag << std::endl;
        std::cout << "Array size: " << headerDxt10.arraySize << std::endl;
        std::cout << "Misc flags 2: " << headerDxt10.miscFlags2 << std::endl;

        int pitchSize = 0;

        if (headerDxt10.dxgiFormat == 24) {
          std::cout << "DXGI_FORMAT_R10G10B10A2_UNORM" << std::endl;
          glFormat = GL_RGB10_A2;
          pitchSize = 4;
        } else if (headerDxt10.dxgiFormat == 25) {
          std::cout << "DXGI_FORMAT_R10G10B10A2_UINT" << std::endl;
          glFormat = GL_RGB10_A2UI;
          pitchSize = 4;
        } else if (headerDxt10.dxgiFormat == 26) {
          std::cout << "DXGI_FORMAT_R11G11B10_FLOAT" << std::endl;
          glFormat = GL_R11F_G11F_B10F;
          pitchSize = 16;
        } else if (headerDxt10.dxgiFormat == 49) {
          std::cout << "DXGI_FORMAT_R8G8_UNORM" << std::endl;
          glFormat = GL_RG8;
          pitchSize = 2;
        } else if (headerDxt10.dxgiFormat == 50) {
          std::cout << "DXGI_FORMAT_R8G8_UINT" << std::endl;
          glFormat = GL_RG8UI;
          pitchSize = 2;
        } else if (headerDxt10.dxgiFormat == 80) {
          std::cout << "DXGI_FORMAT_BC4_UNORM" << std::endl;
          glFormat = GL_COMPRESSED_RED_RGTC1;
          pitchSize = 8;
        } else if (headerDxt10.dxgiFormat == 81) {
          std::cout << "DXGI_FORMAT_BC4_SNORM" << std::endl;
          glFormat = GL_COMPRESSED_SIGNED_RED_RGTC1;
          pitchSize = 8;
        } else if (headerDxt10.dxgiFormat == 82) {
          std::cout << "DXGI_FORMAT_BC5_TYPELESS" << std::endl;
          glFormat = GL_COMPRESSED_RG_RGTC2;
          pitchSize = 16;
        } else if (headerDxt10.dxgiFormat == 83) {
          std::cout << "DXGI_FORMAT_BC5_UNORM" << std::endl;
          glFormat = GL_COMPRESSED_RG_RGTC2;
          pitchSize = 16;
        } else if (headerDxt10.dxgiFormat == 84) {
          std::cout << "DXGI_FORMAT_BC5_SNORM" << std::endl;
          glFormat = GL_COMPRESSED_SIGNED_RG_RGTC2;
          pitchSize = 16;
        } else if (headerDxt10.dxgiFormat == 95) {
          std::cout << "DXGI_FORMAT_BC6H_UF16" << std::endl;
          glFormat = GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT;
          pitchSize = 16;
        } else if (headerDxt10.dxgiFormat == 96) {
          std::cout << "DXGI_FORMAT_BC6H_SF16" << std::endl;
          glFormat = GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT;
          pitchSize = 16;
        } else if (headerDxt10.dxgiFormat == 97) {
          std::cout << "DXGI_FORMAT_BC7_TYPELESS" << std::endl;
          glFormat = GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB;
          pitchSize = 16;
        } else if (headerDxt10.dxgiFormat == 98) {
          std::cout << "DXGI_FORMAT_BC7_UNORM" << std::endl;
          glFormat = GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
          pitchSize = 16;
        } else if (headerDxt10.dxgiFormat == 99) {
          std::cout << "DXGI_FORMAT_BC7_UNORM_SRGB" << std::endl;
          glFormat = GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB;
          pitchSize = 16;
        } else {
          std::cout << "Unknown format" << std::endl;
          return nullptr;
        }

        int totalSize = std::max(1, ((width + 3) / 4)) *
                        std::max(1, ((height + 3) / 4)) * pitchSize;

        std::cout << "Total size: " << totalSize << std::endl;
        std::cout << "dwPitchOrLinearSize: " << header.dwPitchOrLinearSize
                  << std::endl;
        data.resize(totalSize);
        file.read(&data[0], totalSize);
        result->texture.create(width, height);

        sf::Texture::bind(&result->texture);

        glCompressedTexImage2D(GL_TEXTURE_2D, 0, glFormat, width, height, 0,
                               totalSize, &data[0]);

        GLenum error = glGetError();
        if (error) {
          std::cout << std::hex << "Error: " << error << std::endl;
          return nullptr;
        }

        return result;
      } break;
      }

      int totalSize = std::max(1, ((width + 3) / 4)) *
                      std::max(1, ((height + 3) / 4)) * blocSize;
      std::cout << "Total size: " << totalSize << std::endl;
      std::cout << "dwPitchOrLinearSize: " << header.dwPitchOrLinearSize
                << std::endl;
      std::flush(std::cout);

      data.resize(totalSize);

      file.read(&data[0], totalSize);

      result->texture.create(width, height);

      sf::Texture::bind(&result->texture);

      glCompressedTexImage2D(GL_TEXTURE_2D, 0, glFormat, width, height, 0,
                             totalSize, &data[0]);

      GLenum error = glGetError();

      if (error) {
        std::cout << std::hex << "Error: " << error << std::endl;
        return nullptr;
      }
    } else if (header.ddspf.dwFlags & DDPF_RGB) {
      int totalSize = header.dwWidth * header.dwHeight * 4;
      data.resize(totalSize);
      file.read(&data[0], totalSize);
      result->texture.create(width, height);
      sf::Texture::bind(&result->texture);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                   GL_UNSIGNED_BYTE, &data[0]);
      GLenum error = glGetError();
      if (error) {
        std::cout << std::hex << "Error: " << error << std::endl;
        return nullptr;
      }
    } else {
      std::cout << "Unsupported format" << std::endl;
      return nullptr;
    }

    return result;
  }

  return nullptr;
}
