# Icone: D na nuvem

Arte escolhida pelo usuario: proposta 01 da segunda prancha, que combina D azul,
nuvem branca, seta/cartao microSD verde e orbita verde sobre fundo azul escuro.

`app/icon.png` e a arte canonica em PNG RGBA, 300x300, com transparencia ao
redor do quadrado arredondado, sem numero ou legenda. O empacotamento copia
esse arquivo diretamente; `scripts/make_icon.py` apenas valida a estrutura,
sem regenerar o icone anterior de pasta.

O canvas e 300x300, mas a ilustracao foi reduzida para caber numa area de
170x170 a partir de (65,25), deixando o rodape transparente para o nome. Essa
area foi medida diretamente nos icones oficiais de User Guide e PortMaster
da tag CrossMix v1.3.0: limites visiveis (65,25)-(235,195). A imagem final tem
limites alfa (65,25)-(233,195), devido aos pixels suavizados das bordas.

As tentativas anteriores com arte maior/centralizada sobrepunham o titulo, como
mostrou a foto enviada pelo usuario. Corrigir so as dimensoes externas nao era
suficiente. `make_icon.py` e a auditoria do pacote agora verificam tambem a area
visivel e a transparencia inferior. O posicionamento do nome continua sendo
controlado pelo launcher; o resultado final exige conferencia no console.

Para atualizar somente o icone, com o app fechado, extraia
`DriveBrowser-icone-D-na-nuvem-CrossMix.zip` na raiz do microSD e substitua
`Apps/DriveBrowser/icon.png`. O ZIP contem apenas esse arquivo. Atualize a lista
de aplicativos ou reinicie a interface do CrossMix se a imagem antiga estiver
em cache. A exibicao no launcher fisico ainda precisa ser conferida no console.
