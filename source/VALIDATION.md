# PLOC 0.6.2, préparation du 7 novembre 2026

Qualification visée : compilation et tests sur Mac, sans installation matérielle.
Les résultats finaux sont consignés dans la livraison validation.json et checks.log.
La photo de la 0.6.1 montre que le chiffre de SPRAY déborde sur RIPPLE, et
que les valeurs basses et l’onde extérieure touchent le bord utile de l’OLED.
Marge calculée d’après la photo : valeurs à 32, QUALITY à 16, base des ondes
remontée et aplatie de 25 %. PLOCCTRL reste identique à la 0.6.1.
La nouvelle marge doit encore être confirmée sur la machine.

- Sept réglages, trois gestes Shift et Shift + bleu consommé sans aucune écriture.
- Sauvegarde exacte, anciennes valeurs TAIL et autres secondaires préservées.
- Corps opaque et douze orientations obtenues par projection du profil Figma, avec quatre poignées et deux faces réutilisées.
- Sept projections toujours présentes, veille à 20 %, allumage successif.
- CLEAN de 20 à 100 % ; une à quatre ondes, luminosité et perspective indépendantes.
- SIZE / HEIGHT et séparation du libellé et du chiffre contrôlés aux extrêmes.
- Cache de la partie fixe et zéro mutation lors de 500 frames inchangées.
- Ancien rendu et modèles historiques conservés ; nœuds absents tolérés.
- Moteur DSP et stockage inchangés, fichiers source vérifiés par SHA-256.
- Rendu de la preview issu du callback C ; fonte native privée pour les seules épreuves.

Le SVG 0.6.1 réduit la mémoire de dessin en réutilisant les formes de poignée,
en gardant les courbes cubiques compactes et en simplifiant les contours
sous le pixel. Le coût sur Blackfin et l’aspect OLED restent à tester.

## Notes des versions précédentes

# Installation PLOC 0.5.0, 6 novembre 2026

- Firefox : remplacements PLOC et PLOCCTRL 0.4.1 → 0.5.0.
- Plan : 61 autres plugins inchangés, aucune installation d’effet T3.
- Envoi de 27 040 + 1 820 octets, redémarrage T1 observé par le workshop.
- État final UPDATE VERIFIED, OP-1 Hundekraft 3.1.4 et 63 plugins présents.
- Banque existante conservée ; aucune opération DISK ni réimportation.
- Reçu daté et capture enregistrés dans la livraison 0.5.0.
- Apparence OLED, son, gestes Shift, sauvegarde et coût CPU sur instrument
  restent à confirmer par Nicolas. Les contrôles hôte ci-dessous passent.

La section suivante décrit l’état au moment de la préparation, avant installation.

# Validation PLOC 0.5.0, 6 novembre 2026

Version préparée pour OP-1 original / R246 / Hundekraft 3.1.4.
Non installée sur l’instrument à ce stade ; la 0.4.1 reste la version matérielle validée.

- PLOC et PLOCCTRL construits avec Blackfin GCC et le packer officiel.
  API, CRC, imports autorisés et relocalisation à deux adresses vérifiés.
- Huit suites C avec ASan/UBSan et avertissements traités en erreurs :
  modèles historiques, modèle 4, surface intégrée, adaptateur six voix,
  contrôleur, écran moderne et écran historique. Tous passent.
- RIPPLE : quatre paliers, direct et jusqu’à trois trajets sans feedback.
  Mélange convexe lissé ; une onde est exactement identique au rendu 0.4 sec.
  Aucun changement du gain de sortie, aucun nouveau plugin T3.
  520 octets d’état par voix, retard maximal 225 échantillons (5,10 ms).
- Les huit écoutes ont zéro échantillon à la limite du moteur. La nouvelle
  réponse ne dépasse aucun pic correspondant du rendu 0.4 de comparaison.
  L’A/B utilise le même gain ; l’atténuation de la surface reste audible.
- Une vue ; commandes physiques FLOW / SIZE / VISC / RIPPLE et, sous Shift,
  SPRAY / HEIGHT / TAIL / IMPACT. Relâchement sans mode persistant.
  Codec, sélecteur, six macros cachés et ordre interne MIDI/LFO conservés.
- Écran : reflet solidaire de la goutte, point fixe quand VISC varie,
  1–4 ellipses, 0–8 petits traits, perspective indépendante du nombre d’ondes,
  aucune nouvelle géométrie pour TAIL, aucune mutation à l’arrêt.
- 24 états calculés par les vrais callbacks C, puis inspectés dans Firefox.
  SVG conservateur de 7 496 octets ; cinq objets texte R246 retenus.
  Police Iosevka libre pour l’aperçu Mac, texte natif sur l’OP-1.
  24 composants et leurs instances. Le nouveau dessin reprend le croquis de Nicolas.
- Les huit presets existants et leurs effets restent compatibles sans réimport.
  Les fichiers de banque livrés sont réservés aux nouvelles installations.

À vérifier sur la machine : lisibilité OLED, gestes Shift et sauvegarde,
charge CPU avec accords, REVERB/SNOWGLOBE et séquenceur, caractère du nouveau RIPPLE.
Les WAV sont calculés sur Mac avec le DSP entier, sans T2/T3/T4 ni mélangeur natif.
Ce ne sont pas des captations de l’OP-1.

Les sections suivantes gardent l’historique matériel précédent.

# Correction des doublons, 6 novembre 2026

- Huit anciens exports SHA-identiques à ceux d’avant le changement de FX
  appelaient encore le plugin RIPPLE retiré. Le dispatcher les rabat sur le
  filtre natif ; le son brutal signalé est compatible avec ce repli.
- Sauvegarde complète de ces fichiers et des huit sons de touches avant retrait.
- Retrait des huit fichiers anciens, puis éjection et synchronisation native.
- Nicolas : « Huit presets uniques avec effets, DISK prêt ».
- Nouvelle exportation : huit noms uniques, tous les FX actifs corrects,
  ADSR/LFO et mots du synthé conformes aux bonnes versions précédentes.
- 210 autres presets permanents : SHA-256 inchangés. Les fichiers de touches
  sont réexportés par l’OP-1 ; le slot 1 reflète ruissel après l’écoute utilisateur.
- Aucune modification du moteur, du gain ou du réglage des effets conservés.
- Banque native correcte sauvegardée, reçus et archive de livraison mis à jour.

# Validation PLOC 0.4.1, 6 novembre 2026

- PLOC et PLOCCTRL 0.4.1 installés, Firefox UPDATE VERIFIED.
- Retours matériels de Nicolas : « Flow et niveau OK », « Pages et sauvegarde OK ».
- Le précédent effet RIPPLE 0.4.1 avait passé le test de tenue 15–20 s sans
  alerte de lenteur ; il est ensuite retiré à la demande de Nicolas.
- Reconstruction active : uniquement PLOC et PLOCCTRL. Leurs SHA-256 sont
  identiques aux binaires validés sur l’instrument.
- Six contrôles C avec ASan/UBSan : DSP, modèle 3, modèle 4, adaptateur six
  voix, contrôleur natif et écran. Silence, fin du Flow, codec des dix
  secondaires, sauvegarde et compatibilité historique couverts.
- 256 extrêmes du modèle 4 ; pic solo 27465, six voix 20748, zéro clip dans
  ce test. Adaptateur natif : pic 15681, anciens rendus identiques.
- Imports, API, CRC et relocalisation synthétique contrôlés par le SDK 3.1.4.
- Banque de huit fichiers AIFC sans numéros, lus par le parseur indépendant
  du SDK. Plages Spring/Delay/Reverb/Snowglobe vérifiées dans les fichiers.
- Copie native des nouveaux FX relue et comparée aux données préparées.
  Tous les champs de synthèse, d’enveloppe et LFO d’origine sont conservés.
  Les sauvegardes complètes et les changements sont consignés dans le reçu FX.
- Les WAV de la livraison sont secs, sans T2/T3/T4 ni mélangeur natifs.
  Ils ne préjugent pas du niveau et du coût CPU des quatre effets sur l’OP-1.

Voir `../PLOC-0.4.1/validation.json`, `checks.log` et les reçus datés du
6 novembre pour le statut matériel courant. Les sections suivantes
conservent l’historique des versions précédentes.

# Validation PLOC 0.3.0, 6 novembre 2026

- Compilation Blackfin avec le packer officiel SDK 3.1.4 : 12 696 octets.
- SHA-256 : `70c68d2c25ff8cef0433fe243944aaa32c5a91d36fb1a5bc73a24f7ea0b5c189`.
- API 1, imports officiels et deux relocations synthétiques acceptés.
- ASan/UBSan : 2 048 coins note/contrôles, silence final, contrôles en jeu,
  cinq départs de bulles audibles indépendamment, halo RIPPLE et pouic ascendant.
- Toute la plage 0..32767 du rouge est répartie en six valeurs 0..5.
  Le nombre est capturé à la note et n'interrompt pas les bulles déjà lancées.
- Pic du balayage modèle 3 : 21 980 par voix. État 32 octets, 36 avec vtable.
  Cette mesure ne garantit pas l'absence de saturation de six voix sommées.
- Écran : bulles et marqueurs 0..5 immédiats, sans note, retour à zéro,
  autres lectures 0..99, anciennes étiquettes, objets Text retenus, re-entry
  et nœuds absents testés. SVG natif accepté par le contrôle conservateur du kit.
- Ancien DSP 0.1 conservé ; modèle 0.2 compatible avec la correction de gain.
- RAIN inchangé : 8 916 note-on/note-off appariés, aucun événement résiduel.
- Huit AIFC reconnus par le lecteur indépendant du SDK, marqueur modèle 3.
- Aperçu interactif vérifié dans Firefox, cinq bulles affichées au maximum.
  La police Mac est une approximation Iosevka de la police native.
- Installation physique : Firefox affiche UPDATE VERIFIED après redémarrage,
  PLOC 0.2.0 → 0.3.0, 63 plugins, 62 inchangés.
- Huit nouveaux presets copiés, SHA-256 vérifiés par relecture, zéro écrasement,
  226 → 234 fichiers synth, huit anciens PLOC conservés, disque éjecté.

Nicolas a confirmé l'import et la lecture avec 11 pouic : écran, bulles du rouge
et niveau sonore OK sur l'OP-1.
Les performances CPU en polyphonie avec les FX restent à examiner. Les WAV
proviennent du code C sur Mac, sans enveloppe T2, FX ni mélangeur natifs.
Voir `../INSTALLATION-PLOC-0.3.0-2026-11-06.json` pour les confirmations suivantes.
Les sections ci-dessous sont l'historique avant cette installation.

# Validation PLOC 0.2.1

0.2.1 : art Figma 4:535 porté en SVG natif (2073 octets), huit objets Text
R246 retenus, quatre lectures 0..99, construction unique même après re-entry,
chaînes inchangées non remises à jour, animation et nœuds absents vérifiés
avec mocks. ASan/UBSan et compilation Blackfin passent ; CRC/imports/relocations
vérifiés par le SDK. Le nouveau modèle est relevé de 12,04 dB, pic source
par voix mesuré 18828. La borne antérieure de somme de six voix brutes n'est
plus revendiquée. Polyphonie, niveau et police sur OLED restent à examiner
sur l'instrument. Les huit AIFC sont identiques à ceux importés avec 0.2.0.

0.2.0 : installation vérifiée dans Firefox ; import et lecture des huit presets
confirmés par Nicolas, qui a signalé le niveau trop faible et l'ancien écran.
Voir les journaux `../INSTALLATION-PLOC-0.2.0-2026-11-05.json` et les preuves
Firefox dans le dossier parent.

Les sections suivantes conservent les contrôles historiques du portage 0.2.0
et du prototype 0.1, avant les changements d'interface et de gain.

# Validation PLOC 0.2 et état des versions

PLOC 0.2.0 et sa banque de huit presets sont dans `../PLOC-0.2.0/`.
Contrôles passés le 5 novembre 2026 : compilation Blackfin par le packer
SDK 3.1.4, CRC/API/imports, deux relocations synthétiques, huit AIFC natifs
avec identité `ploc` et marqueur de modèle 2, ainsi que les tests DSP
ASan/UBSan. 2048 combinaisons note/contrôles, pic mesuré 4707 par voix,
vtable et limite de 36 octets respectées, retour au silence et anciens sons
équivalents échantillon pour échantillon au DSP 0.1. Erreur maximale du
coefficient d'amortissement 1,527 % entre 80 et 8000 Hz ; erreur RMS relative
de la bulle simple face à l'étude analytique 2,524 %.

Les presets sont lisibles par le lecteur SDK indépendant. Les écoutes sont
le DSP sec sur Mac, sans T2/T3/T4 natifs. Voir `checks.log` et `validation.json`
dans la livraison. Installation 0.2, import natif et CPU matériel restent à
tester. Le nouveau design Figma n'est pas encore porté.

PLOC 0.1 et RAIN 0.1 ont depuis été installés et confirmés fonctionnels par
Nicolas. Les observations ci-dessous décrivent les contrôles **avant** cette
installation et restent un historique de la première version.

# Validation — 2026-11-05

## Passed locally

- Official SDK 3.1.4 archive SHA-256 matched the published checksum before extraction.
- C DSP: AddressSanitizer and UndefinedBehaviorSanitizer, fail on first error.
- 2,048 PLOC note/extreme-control combinations (128 pitches × 16 control corners).
- PLOC tail-energy comparison and decay to silence; tested sample bound below
  5,461, leaving room for six voices in a signed 16-bit mix.
- RAIN: 8,916 successful note-ons paired with exactly one note-off each.
- RAIN: silent density zero and opening zero; greater density/opening yields
  more triggers using identical deterministic initialization.
- RAIN: last-key release, repeated key activity, stop/reset and failed output
  allocation do not leave owned notes; test peak four voices, hard bound six.
- Source checked with a freestanding 32-bit Clang target to verify C syntax
  and target-size assertions. This check does not generate Blackfin code.
- Both original SVG files passed the OP-1 Design Kit conservative-source check.
- Offline WAV fixtures generated from production C DSP/event model.

## Still required on OP-1

- Plugin load/registration on the user's Hundekraft 3.1.4 core.
- Native SVG parsing, moving-node transforms, text placement and physical readability.
- Actual sound, velocity, T2/T3/T4 interaction, peak CPU under six voices and heavy FX.
- Transport/MIDI start, repeated notes, octave and sound/slot/mode changes.
- RAIN synth/drum modes, persisted controls, restart and interrupted save.
- Saved PLOC preset reload; plugin OFF, removal, missing-provider fallback and reinstall.

No device connection or installation was performed. The Mac WAV envelope is
not a recording of the OP-1's T2 envelope or a full native emulator run.

## Blackfin packaging passed

- Built GNU binutils 2.38 and bfin-elf GCC 13.3.0 locally; no system tool installation.
- Official Kennel packer produced `ploc.knl` (3,136 bytes) and `rain.knl` (7,560 bytes).
- Official plugin parser accepted both with API 1 and valid CRC.
- Every imported symbol exists in the SDK 3.1.4 release export table.
- Official relocator passed at two synthetic load bases using far-call stubs
  and synthetic core export addresses. This is a format/relocation check,
  not native execution or proof of the live device's export addresses.
- File hashes and dependency lists are in `output/package-check.json`.

