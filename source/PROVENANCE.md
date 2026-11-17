# Sources

- Official workshop repository: https://github.com/Hundekraft/op1-workshop
- Revision: `1cb2e0be590e2a82d82f9ad6f9b3410bc97b7fae`
- SDK: `hundekraft-3.1.4-source.tar.gz (downloaded from upstream)`, checked against its adjacent SHA-256 file before extraction.
- OP-1 Design Kit repository: https://github.com/squarewave-studio/op1-design-kit
- Design-kit revision: `ca96a17189ca2785d34b2c087459d2a17556ded6`

PLOC DSP, RAIN event model and the drop/umbrella drawings are original work
for Nicolas's concept. Native descriptors, screen/persistence helpers and
player vtable organization follow the Kennel SDK templates and c64arp example
(Copyright 2026 Hundekraft; API copyright 2026 pcppcp; shared stroke helpers
copyright 2026 ultralazr). License notices are retained in NOTICE.
Design guidance: SquareWave Studio OP-1 Design Kit, MIT; no kit glyph artwork
or TE font is embedded. No Teenage Engineering firmware or keys included.

PLOC 0.5 faucet/drop drawing and physical-knob mapping follow Nicolas's
selected 6 November 2026 sketch. The short feed-forward surface response is
original synth code, not the retired RIPPLE T3 plugin.
The host preview includes unmodified Iosevka Thin/Light under the bundled OFL;
no font is embedded in either .knl file. The included design-kit subset is
limited to its SVG checker, font files and license notices.

## PLOC 0.6.0, 7 novembre 2026

Design utilisateur : dessins originaux de Nicolas pour PLOC. Les géométries source téléchargées restent inchangées dans
plugins/ploc/figma-assets/v6. L'adaptation utilise le lecteur de courbes du SDK,
une simplification bornée et le profil SVG R246. Les textes interactifs utilisent
les objets Text natifs ; les graphismes CLEAN et H2O sont ceux du dessin fourni.
Les couleurs fonctionnelles suivent les instructions du 7 novembre.

Le DSP 0.5 et le codec 0.4 sont conservés sans modification. La livraison contient
uniquement PLOC et PLOCCTRL, les sources, les contrôles et des aperçus. Aucune
nouvelle banque de presets, police TE complète ou image de firmware n'est livrée.

## PLOC 0.6.1

Allègement de l’écran pour la configuration de 63 plugins. Les sources Figma
restent inchangées ; le profil des poignées est projeté en douze orientations
sans charger douze dessins distincts. Moteur sonore et presets inchangés.

## PLOC 0.6.2

Ajustement de la typographie et de la position des ondes d’après la photo OLED
fournie par Nicolas. Les mesures sont décrites dans oled-photo-layout.json.
La photo et la police privée ne sont pas incluses.
