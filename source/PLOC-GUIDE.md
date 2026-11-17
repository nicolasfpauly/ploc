# PLOC 0.6.2

OP-1 original, R246, Hundekraft 3.1.4. Préparé le 7 novembre 2026.

| Potard | Tourner | Shift + tourner |
|---|---|---|
| Bleu | FLOW | Réservé, aucun changement |
| Vert | SIZE | HEIGHT |
| Blanc | SPRAY | QUALITY |
| Rouge | RIPPLE | IMPACT |

Une seule vue. Les libellés secondaires apparaissent pendant le geste Shift et reviennent au relâchement.

FLOW entraîne ensemble le débit sonore, l'aiguille du tachymètre et les douze positions du croisillon. Le corps et les raccords restent fixes, avec leur intérieur noir opaque.

SIZE change les dimensions de la goutte. HEIGHT la monte ou la descend et conserve l'action sonore de l'ancien POUIC. La partie haute de la goutte peut passer derrière le robinet, comme dans le dessin Figma.

SPRAY conserve les sept petites gouttelettes à l'écran. À zéro, elles sont grises à 20 % ; elles s'allument progressivement jusqu'à être toutes blanches au maximum. Le réglage conserve son action sonore existante. Son stockage historique offre seize positions.

QUALITY reprend le réglage sonore et le mot de stockage de l'ancienne viscosité. Visuellement, seul le badge CLEAN varie entre 20 et 100 % de luminosité. La goutte et sa forme ne changent pas avec QUALITY.

RIPPLE règle une à quatre ondes. IMPACT règle leur luminosité entre 20 et 100 % et change légèrement leur perspective, environ ±8 % en hauteur. IMPACT conserve aussi son rôle sonore existant.

Les valeurs affichées vont de 00 à 99. Le nombre d'ondes reste quantifié en quatre niveaux. Le noir et les niveaux de gris sont des couleurs opaques, sans couche de transparence globale.

## Sons et presets

Le moteur sonore est identique à la 0.5.0. Le gain, les protections et les effets des presets existants sont conservés. Le paramètre TAIL et les autres anciens secondaires restent enregistrés ; cette interface ne les modifie pas.

Le format des huit mots de preset reste compatible avec la 0.4/0.5. Aucune banque n'est générée ni à recopier. Les presets existants ne doivent pas être réimportés.

L'ordre brut MIDI/LFO reste SIZE / RIPPLE / QUALITY (ancienne VISC) / FLOW. Le contrôleur réordonne les gestes des potards physiques ; SPRAY reste une macro stockée séparément.

## Fichiers et mise à jour

Les deux fichiers à mettre à jour ensemble sont ploc.knl et plocctrl.knl. Ils remplacent les plugins portant ces mêmes noms. Aucun effet T3 ni séquenceur supplémentaire n'est livré.

La préparation ne se connecte pas à l'instrument et n'installe rien. Pour l'essai matériel, utiliser Firefox et le workshop Hundekraft 3.1.4, conserver les autres plugins du plan et suivre ses instructions d'application. Les fichiers de la 0.5.0 restent archivés pour le retour à la version précédente.

## Vérification

Compilation Blackfin, imports/API, relocalisation à deux adresses, huit harnesses sous ASan/UBSan, sauvegarde des mots de preset, gestes Shift, limites des paramètres, anciens modèles, SVG natif et 29 états de l'écran vérifiés sur Mac.

Le rendu de preview utilise les états du vrai callback C et une simulation de la typographie native. Il ne constitue pas une capture OLED. Le coût de dessin et le rendu sur l'OP-1 restent à valider. Le SVG reste plus petit que celui installé en 0.6.1. La poignée garde ses douze crans avec les mêmes six formes. Si la 0.6.1 est installée, seul ploc.knl doit être remplacé.

## Sources

Le design suit le fichier Figma de Nicolas, écran 49:9400, avec les couleurs des commandes demandées le 7 novembre. Les téléchargements d'origine et les matrices de placement sont conservés dans source/plugins/ploc/figma-assets/v6 et source/design/figma-0.6.

Les textes interactifs utilisent la police native R246. Aucun fichier de police TE complet ni firmware n'est inclus.
