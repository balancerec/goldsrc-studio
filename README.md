# GoldSrc Studio

Editor desktop de texturas **WAD3** para a engine GoldSrc e **Counter-Strike 1.6**, desenvolvido em **C++20 + Qt 6 Widgets**, com interface em português brasileiro.

> **Status: desenvolvimento / pré-lançamento.** Utilize cópias dos arquivos WAD3 originais. Os recursos descritos aqui existem na branch de desenvolvimento `docs/current-editor-guide` e suas branches ancestrais; podem ainda exigir validação funcional. Não são uma declaração de que estão disponíveis na `main`.

## Funcionalidades

- Abrir WAD3 e listar texturas MIPTEX não compactadas (`0x43`) com miniaturas e filtro por nome.
- Importar imagens PNG, JPG, JPEG e BMP; importar arquivos por drag-and-drop (recurso sujeito a validação nos diferentes painéis).
- Duplicar, excluir, renomear e redimensionar texturas.
- Pincel, borracha, conta-gotas, preenchimento de área, seleção retangular e varinha mágica com tolerância RGBA.
- Recorte por coordenadas, recorte de seleção, remoção de regiões de fundo por cor e conversão de cor em transparência.
- Seleção com alças para mover e alterar seus limites.
- Transformar os **pixels selecionados**: pré-visualizar movimento e redimensionamento, confirmar ou cancelar.
- Desfazer/refazer: até 15 operações na textura ativa. O histórico é reiniciado ao trocar de textura.
- Exportar WAD3 com quatro níveis de mipmap e paleta indexada de 256 entradas.
- Preservar os dados MIPTEX originais, inclusive paleta e mipmaps, quando a textura não foi modificada.

## Uso do editor

1. Abra um arquivo WAD3 em **Abrir WAD3**, ou importe imagens.
2. Selecione uma textura na biblioteca à esquerda.
3. Escolha a ferramenta acima do canvas.
4. Ajuste zoom, tolerância da varinha mágica, cor e grade.
5. Para selecionar pixels, escolha **Seleção retangular** ou **Varinha mágica**.
6. Para transformar os pixels, use **Seleção → Mover / transformar pixels selecionados**. Arraste a região ou suas alças e confirme com **Enter**; cancele com **Esc**.
7. Exporte para um **novo arquivo WAD3**. Recomenda-se verificar o resultado no jogo antes de substituir ativos.

### Navegação com mouse

| Gesto | Resultado |
| --- | --- |
| Clique esquerdo + arrastar, com pincel/borracha | Edita a textura |
| Clique esquerdo + arrastar, com seleção retangular | Desenha a seleção com prévia |
| Arrastar o interior da seleção retangular | Move o **limite da seleção**, sem mover seus pixels |
| Arrastar as alças da seleção | Redimensiona o limite da seleção |
| **Seleção → Transformar**, depois arrastar | Move ou redimensiona os **pixels selecionados** |
| **Ctrl + roda** | Zoom de 1× a 32× com tentativa de manter o ponto sob o cursor |
| **Botão central + arrastar** ou **Alt + botão esquerdo + arrastar** | Navegar pela área de trabalho |
| **Ctrl+Z / Ctrl+Y** | Desfazer/refazer |
| Arrastar arquivos para a janela | Importação de imagens/WAD3 (validar conforme painel de destino) |

## WAD3, paleta e transparência

- Texturas intactas vindas de WAD3 podem ser reexportadas com o conteúdo MIPTEX original, preservando índices, mipmaps e cores.
- Para texturas **editadas**, o exportador usa uma paleta adaptativa baseada nas **255 cores mais frequentes**, aproximando cores excedentes pela distância RGB. Isso evita a antiga quantização fixa RGB332, mas ainda pode alterar fotos e gradientes com mais de 255 cores.
- A transparência da engine GoldSrc é representada pelo **índice 255** em texturas com nome iniciado por `{`, por exemplo `{GRADE`. Em texturas sem esse prefixo, a transparência alfa **não é preservada como transparência GoldSrc**.
- O exportador usa limiar de alfa na conversão e gera quatro mipmaps. Texturas editadas podem ter dimensões ajustadas para múltiplos de 16.
- Nome MIPTEX limitado a 15 caracteres nesta implementação. Nomes internos como `STEEL.01` e `{GRADE` são preservados durante a normalização, quando válidos.

## Compilar no Windows (CMD)

Exemplo para uma instalação Qt 6.12.0 / MinGW 13.1, compatível com o ambiente de desenvolvimento utilizado:

```cmd
cd /d C:\Users\balancerec\Documents\goldsrc-studio

git fetch origin
git switch --track origin/docs/current-editor-guide

set "PATH=C:\Qt\6.12.0\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;%PATH%"

cmake -S . -B build-mingw -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="C:\Qt\6.12.0\mingw_64"
cmake --build build-mingw --parallel 4
ctest --test-dir build-mingw --output-on-failure
build-mingw\GoldSrcStudio.exe
```

Se a branch local já existir, substitua `git switch --track ...` por `git switch docs/current-editor-guide` seguido de `git pull --ff-only`.

Para distribuição independente de uma instalação Qt, use `windeployqt` do mesmo kit do compilador.

## Compilar no Linux

Requer Qt 6.4+ (Core, Gui, Widgets), CMake, Ninja ou Make e um compilador C++20.

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
./build/GoldSrcStudio
```

## Testes e integração contínua

O teste `wad3_tests` verifica leitura e escrita, round-trip binário de texturas originais, renomeação, edição, cores exatas em imagem de três cores, transparência por nome iniciado em `{`, opacidade sem o prefixo e rejeição de nomes duplicados.

O workflow `.github/workflows/build.yml` executa CMake/CTest em **Linux (Qt 6)** e **Windows (Qt 6/MSVC)**. Em caso de sucesso no Windows, publica um artefato experimental com o executável e bibliotecas Qt. Testes automatizados **não substituem** validar seleção, drag-and-drop, pan, zoom e transformação com o mouse.

## Estrutura do código

- `src/core/Wad3.h` e `src/core/Wad3.cpp`: leitura e exportação de WAD3 e quantização.
- `src/ui/MainWindow.h` e `src/ui/MainWindow.cpp`: interface, menus, biblioteca, inspetor e importação.
- `src/ui/PixelCanvas.h` e `src/ui/PixelCanvas.cpp`: ferramentas de pixel, seleção, navegação no canvas e histórico.
- `tests/wad3_tests.cpp`: testes de regressão do formato.
- `.github/workflows/build.yml`: compilação e testes Linux/Windows.

## Limitações e próximos passos

- A leitura suporta texturas MIPTEX não compactadas reconhecidas. Outros tipos de lump não fazem parte do fluxo principal.
- Não há sistema completo de camadas, laço, filtros profissionais ou remoção de fundo por IA.
- Histórico por **textura ativa** somente; troca de textura reinicia Undo/Redo.
- A transformação faz recorte nos limites do canvas. Mover pixels deixa transparência na origem.
- A quantização adaptativa atual usa frequências de cores e distância RGB; não utiliza median-cut ou dithering perceptual.
- Exportações modificadas devem ser revisadas no GoldSrc; o suporte a transparência e mipmaps merece testes com diferentes materiais reais.
- Reunir e revisar as PRs encadeadas antes do merge na `main`.

## Licença

MIT — consulte `LICENSE`.
