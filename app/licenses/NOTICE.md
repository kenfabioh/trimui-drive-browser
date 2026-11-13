# Componentes redistribuidos

Drive Browser: GPL-3.0 (LICENSE na raiz, fonte correspondente em source/).
Icone original do projeto, sob a mesma licenca.

* SDL 2.26.1 GE8300: zlib, fornecido pelo SDK oficial TrimUI 20231018.
  Fonte exata e configuracao de build: https://github.com/trimui/toolchain_sdk_smartpro/releases/tag/20231018
  (SDL2-2.26.1.GE8300.tgz). Biblioteca dinamica, substituivel em lib/.
* SDL_ttf: zlib, binario do SDK oficial (TTF_Linked_Version informa 2.0.13;
  cabecalhos/base 2.0.12). Referencia upstream da base, nao garantia de fonte
  exata das alteracoes do fornecedor:
  https://github.com/libsdl-org/SDL_ttf/tree/release-2.0.12
* FreeType 2.6.1: FreeType License, por David Turner, Robert Wilhelm e Werner Lemberg.
  Este software utiliza a FreeType library. Fonte: https://github.com/freetype/freetype/tree/VER-2-6-1
* zlib 1.2.8: zlib, https://github.com/madler/zlib/tree/v1.2.8
* bzip2 1.0.6: licenca bzip2, Julian Seward,
  https://github.com/libarchive/bzip2/tree/bzip2-1.0.6
* libgcc_s e runtimes estaticos GCC/Linaro 7.5.0: GPL-3.0 com GCC Runtime
  Library Exception 3.1. Toolchain oficial TrimUI, fontes GCC:
  https://github.com/gcc-mirror/gcc/tree/releases/gcc-7.5.0
* rclone 1.69.1: MIT. Binario oficial ARM64 sem modificacoes,
  https://github.com/rclone/rclone/tree/v1.69.1
* rclone-page: fonte rclone 1.69.1 modificado pela extensao Drive Browser MIT,
  compilado estaticamente com Go 1.24.0. Fonte da extensao e licenca em
  source/third_party/rclone-page/; patch e hashes em source/scripts/build_pager.py.
  Licencas dos modulos compilados e runtime Go em licenses/pager/ no pacote.
* nlohmann/json 3.11.3: MIT, fonte vendorizada em source/third_party/json.hpp.
* DejaVu Sans: Bitstream Vera/DejaVu, ver DejaVu.txt.
* Certificados Mozilla via certifi 2025.01.31: MPL-2.0,
  https://github.com/certifi/python-certifi/tree/2025.01.31

Nenhuma biblioteca glibc ou driver GPU do firmware e redistribuido.
SDL_ttf 2.0.15 usado SOMENTE nos testes nativos de desenvolvimento: zlib,
https://github.com/libsdl-org/SDL_ttf/tree/release-2.0.15.
QEMU 7.2 usado SOMENTE para testes: nao faz parte do pacote do console.
