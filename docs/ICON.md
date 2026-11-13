# Icone: D na nuvem

Arte escolhida pelo usuario: proposta 01 da segunda prancha, que combina D azul,
nuvem branca, seta/cartao microSD verde e orbita verde sobre fundo azul escuro.

`app/icon.png` e a arte canonica em PNG RGBA, 300x300, com transparencia ao
redor do quadrado arredondado, sem numero ou legenda. O empacotamento copia
esse arquivo diretamente; `scripts/make_icon.py` apenas valida a estrutura,
sem regenerar o icone anterior de pasta.

Na versao 1.0, a arte de 256x256 foi centralizada num canvas transparente de
300x300, sem ampliacao ou redesenho. As margens adicionais reduzem a ocupacao
visual quando o launcher escala a imagem para uma area fixa. O posicionamento
do nome e controlado pelo launcher CrossMix; conferir o alinhamento no console.

Para atualizar somente o icone, com o app fechado, extraia
`DriveBrowser-icone-D-na-nuvem.zip` na raiz do microSD e substitua
`Apps/DriveBrowser/icon.png`. O ZIP contem apenas esse arquivo. Atualize a lista
de aplicativos ou reinicie a interface do CrossMix se a imagem antiga estiver
em cache. A exibicao no launcher fisico ainda precisa ser conferida no console.
