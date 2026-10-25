# GoldSrc Studio

Editor desktop de texturas **WAD3** para GoldSrc / Counter-Strike 1.6, escrito em **C++20 + Qt 6 Widgets**. Interface em português brasileiro.

> **Versão inicial 0.1.0 (MVP).** Código em evolução: use cópias dos arquivos originais. O exportador converte cores para uma paleta RGB332 simplificada e pode alterar a aparência de imagens de entrada. Ainda não é um substituto completo do Wally.

## Recursos atuais

- Abrir WAD3 e listar texturas MIPTEX (`0x43`) não compactadas;
- Importar PNG, JPG e BMP; visualizar miniaturas e textura selecionada;
- Buscar por nome, duplicar, excluir, renomear e redimensionar;
- Exportar WAD3 com quatro níveis de mipmap e paleta de 256 entradas;
- Interface com painéis acopláveis e tema escuro;
- Teste automatizado de gravação e leitura WAD3.

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

- O leitor suporta somente texturas não comprimidas do tipo `0x43` com paleta interna válida. Outros lumps são ignorados.
- Ao reexportar um WAD3, as texturas são reconvertidas; a paleta original **não é preservada**.
- Nomes são normalizados para um subconjunto ASCII, limitados a 15 caracteres úteis (16 bytes de campo).
- O exportador ajusta dimensões a múltiplos de 16, até 4096 pixels por eixo, e usa paleta RGB332 aproximada.
- Undo/redo, desenho por pixel, paleta editável e preview 3D ainda não foram implementados.

## Roadmap

1. Preservar paletas e pixels indexados originais ao importar WAD3;
2. Aperfeiçoar quantização (median-cut e dithering);
3. Editor zoom/pan com grade, seleção e pincéis;
4. Sistema undo/redo e salvamento de projetos;
5. Instalador para Windows e releases automatizadas.

## Licença

MIT — veja `LICENSE`.
