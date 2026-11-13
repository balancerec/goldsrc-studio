# GoldSrc Studio

Editor desktop de texturas **WAD3** para GoldSrc / Counter-Strike 1.6, escrito em **C++20 + Qt 6 Widgets**. Interface em português brasileiro.

> **Versão inicial 0.1.0 (MVP).** Código em evolução: use cópias dos arquivos originais. Imagens editadas são quantizadas para uma paleta adaptativa e podem mudar de aparência.

## Recursos atuais

- Abrir WAD3 e listar texturas MIPTEX (`0x43`) e decals (`0x40`) não compactados;
- Importar PNG, JPG e BMP; visualizar miniaturas e textura selecionada;
- Buscar por nome, duplicar, excluir, renomear e redimensionar;
- Exportar WAD3 com quatro níveis de mipmap e paleta de 256 entradas;
- Interface com painéis acopláveis e tema escuro;
- Teste automatizado de gravação e leitura WAD3.
- Edição por pixel, seleção, transformação, remoção de fundo, zoom e histórico local;
- Modo **Spray CS 1.6** com prévia quantizada, transparência automática, ajuste proporcional e limite de 20.480 bytes;
- Exportação atômica, instalação com backup e hashes SHA-256 e restauração de sprays.

## Criar e instalar um spray

1. Importe PNG com transparência (ou remova o fundo no editor) e selecione a imagem.
2. Clique **Spray CS 1.6**. Confira a prévia real da exportação, dimensões e tamanho.
3. Se o servidor restringir sprays grandes, marque a opção de compatibilidade 64×64.
4. Exporte `tempdecal.wad` ou clique **Instalar no CS** e selecione a pasta `cstrike` que contém `liblist.gam`.
5. Feche o jogo antes de instalar; depois abra e reconecte. No Windows, o instalador detecta `hl.exe`, `cs.exe` e `cstrike.exe` da instalação selecionada. Em outras plataformas, feche o jogo manualmente.

Backups ficam em `cstrike/spray-backups`, com manifesto e hashes. **Restaurar backup** instala um WAD anterior validado e também salva a versão atual. Arquivos somente leitura devem ser desbloqueados nas propriedades antes da instalação. O instalador não apaga caches nem altera `decals.wad`. Selecionar outro spray nas opções do jogo pode substituir a escolha.

O exportador genérico continua adequado a pacotes de texturas; para sprays, use o modo dedicado. O suporte a formatos adicionais como TGA depende dos plugins de imagem Qt instalados.

## Compilar no Windows (Qt Creator recomendado)

1. Instale **Qt 6.4+** incluindo **Qt Widgets** e um kit de compilação correspondente (MSVC ou MinGW), além de CMake.
2. No **Qt Creator**, abra `CMakeLists.txt` e escolha o kit Qt Desktop.
3. Compile e execute `GoldSrcStudio`.

### Terminal (PowerShell, kit Qt configurado)

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/Qt/6.8.3/msvc2022_64"
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Ajuste o caminho para sua instalação Qt. No Windows, distribua o `.exe` com suas DLLs/plugins Qt usando `windeployqt` compatível com o kit.

### Linux (dependências Qt 6 instaladas)

```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/GoldSrcStudio
```

## Estrutura

- `src/core/Wad3.*`: leitura e escrita do formato WAD3;
- `src/ui/MainWindow.*`: interface Qt Widgets;
- `tests/`: teste de round-trip;
- `.github/workflows/`: CI de compilação e teste Linux.

## Limitações conhecidas

- O leitor suporta tipos `0x43` e `0x40` não comprimidos com quatro mipmaps contíguos e paleta de 256 cores. Outros lumps são ignorados.
- Texturas WAD3 **não modificadas** preservam os dados MIPTEX. Após editar os pixels, são selecionadas as cores mais frequentes, aproximando as restantes por distância RGB.
- Nomes são normalizados para um subconjunto ASCII, limitados a 15 caracteres úteis (16 bytes de campo).
- O exportador genérico ajusta dimensões a múltiplos de 16, até 4096 pixels por eixo. Transparência de textura genérica exige prefixo `{`; no modo spray ela é automática.
- O histórico de desfazer ainda é local à seleção atual; trocar de textura o reinicia. Paleta editável, salvamento de projetos e preview 3D ainda não foram implementados.

## Roadmap

1. Preservar paletas e pixels indexados originais mesmo após edição, com editor indexado;
2. Aperfeiçoar quantização (median-cut e dithering);
3. Histórico persistente por textura e aviso de alterações não salvas;
4. Salvamento de projetos;
5. Instalador para Windows e releases automatizadas.

## Licença

MIT — veja `LICENSE`.
