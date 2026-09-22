# ChallengeDXCC C++ — V0.1

Première version C++ corrigée à partir de la transcription initiale de la branche Python V0.10.x.

## Corrections V0.1

- Le tableau **DX SPOTS** est maintenant alimenté par les alertes acceptées provenant de HamAlert, PSK Reporter et DX Cluster, indépendamment de la présence d'un client Logger32 connecté.
- Colonnes conformes à l'interface : **Vu | DX | Pays | Bande | Mode | SNR | Fréq.**
- Agrégation d'un même `CALL + bande` pendant 10 minutes avec compteur **Vu** et conservation du meilleur SNR ; une seule ligne reste visible par indicatif/bande.
- Purge automatique des spots âgés de plus d'une heure.
- Filtre radio par bande conservé et appliqué aussi aux nouveaux spots.
- Tri du tableau par clic sur les en-têtes, avec tri numérique correct pour **Vu**, **SNR** et **Fréq.**
- Après un QSO Logger32/JTDX dans la période Challenge, tous les spots du même **DXCC + bande** sont retirés immédiatement.
- Carte de service **JTDX UDP** reliée à l'état réel du listener UDP.
- Port du cluster local remis à **7301**, identique à la branche Python de référence.
- Correction de la copie post-build de `gui/styles.qss` : le répertoire `gui` est créé avant la copie.

## Étape suivante

La V0.1 ne porte pas encore l'automatisation Windows UIA de JTDX (DX Call, FT8/FT4, Attente) ni Tune Auto TCI. Ces fonctions restent volontairement séparées pour la prochaine étape afin de valider d'abord le flux réseau, la base Challenge et le tableau DX SPOTS en C++.

---

# Challenge DXCC — Portage C++ / Qt6 / CMake

Portage du projet Python/PySide6 `ChallengeDXCC_QtPython_v0_10_2` vers
C++17 / Qt 6 (Widgets, Network, Sql, WebSockets), avec CMake pour Qt Creator.

## État du portage

### Entièrement porté
- **Cœur métier** (`core/`) : modèle de profil Challenge, parseur ADIF,
  résolveur DXCC (préfixes/alias), moteur de Challenge (bandes travaillées/
  manquantes). Fidèle à `core/*.py`.
- **Base de données** (`db/challengedb.*`) : SQLite via QtSql, upsert QSO,
  détection de doublons, agrégats par bande/DXCC. Fidèle à `ChallengeDB`
  (`services/v10_backend.py`).
- **Services réseau** (`backend/`), tous pilotés par signaux Qt plutôt que
  par un `poll()` manuel :
  - `hamalertclient` — Telnet + JSON vers hamalert.org
  - `localdxclusterserver` — serveur DX Cluster local pour Logger32
  - `pskreportermqttclient` — client MQTT 3.1.1 **minimal écrit à la main**
    (Qt n'embarque pas de client MQTT) pour `mqtt.pskreporter.info`
  - `genericdxclusterclient` — client DX Cluster Telnet générique
    (préférence spotters européens)
  - `jtdxudplistener` — réception UDP JTDX (header binaire + ADIF brut)
  - `logger32bridge` — pont Win32 natif avec Logger32 (remplace le module
    `ctypes` Python ; compile aussi hors Windows, en mode "indisponible")
  - `challengebackend` — orchestrateur qui remplace à la fois
    `CommunicationsHub` (le lanceur de sous-processus Python) et
    `Logger32ChallengeV9` : tout tourne nativement dans le process Qt, plus
    de sous-processus ni de parsing de stdout.
- **Interface graphique** (`gui/`) : `main_window.ui` est repris tel quel
  (le format Designer est identique pour PySide et Qt/C++) et chargé
  nativement par `uic` (`Ui::ChallengeDXCCWidget`). `MainWindow` porte la
  logique de `gui/main_window.py` : profil, tableaux bande/manquants,
  cartes de service, DX Spots, journal filtrable, persistance `QSettings`.
  `TciClient` porte `PersistentTCIClient` (QSY instantané par double-clic,
  WebSocket TCI).

### Non porté (assumé, documenté)
- **Tune Auto TCI** n'est **pas encore** porté. Dans la branche Python,
  Tune Auto pilote ExpertSDR directement par TCI (`tune`, `tune_drive`,
  télémétrie PWR/ROS) ; il ne dépend pas de JTDX. Il est donc portable
  proprement en C++ avec `QWebSocket`, mais sera ajouté après validation de
  cette V0.1.
- **Automatisation JTDX** (DX Call, contrôle FT8/FT4, `Attente <CALL>`)
  n'est pas encore portée. Elle nécessite l'API Windows UI Automation et
  sera traitée séparément de Tune Auto.
- Quelques détails très fins du journal Python (durées de vie différenciées
  par type de message, purge périodique à la seconde près) ont été
  simplifiés à une politique unique de purge par nombre de lignes
  (`MAX_LOG_ROWS`). L'essentiel — filtres par catégorie, sauvegarde,
  horodatage — est conservé.

## Validation de compilation

Le projet a fait l'objet d'une vérification statique de cohérence dans le
bac à sable, mais Qt6 n'y est pas installé : une compilation/link complète
ne peut donc pas être exécutée ici. La première compilation réelle doit être
faite dans Qt Creator avec votre kit Qt 6 Windows.

Le pont Logger32 (`backend/logger32bridge.cpp`) utilise l'API Win32 native
(remplace `ctypes`) ; il ne peut être compilé en mode fonctionnel que sous
Windows (`CHALLENGEDXCC_WINDOWS` défini par CMake quand `WIN32`). Sur les
autres plateformes, il compile mais reste inactif.

## Construire avec Qt Creator

1. Ouvrir `CMakeLists.txt` dans Qt Creator ("Ouvrir un projet").
2. Configurer avec un kit Qt 6.5+ (Widgets, Network, Sql, WebSockets).
3. Compiler. Les dossiers `resources/`, `config/`, `profiles/`, `gui/styles.qss`
   et `runtime/` sont copiés à côté de l'exécutable après compilation.

## Construire en ligne de commande

```bash
cmake -B build -DCMAKE_PREFIX_PATH=/chemin/vers/Qt/6.x.x/gcc_64
cmake --build build
```

## Différences d'architecture par rapport à la version Python

- Plus de sous-processus : le Python lançait `services/v10_backend.py`
  comme process séparé et parsait sa sortie standard ligne par ligne
  (`services/communications.py`, `QProcess` + regex). En C++, tous les
  services (HamAlert, PSK Reporter, DX Cluster, cluster local, UDP JTDX,
  Logger32) sont des `QObject` qui tournent dans le même process que la
  GUI et communiquent par signaux Qt — plus rapide, plus simple à déboguer,
  et sans dépendance à un interpréteur Python.
- Boucle de polling Python (`while True: poll(); sleep(0.05)`) remplacée
  par des sockets Qt asynchrones (`QTcpSocket`, `QUdpSocket`, `QTcpServer`)
  pilotés par signaux `readyRead`/`connected`/`disconnected`, plus des
  `QTimer` pour les tâches périodiques (reconnexion, surveillance du fichier
  de suppressions Logger32).
- Le pont Logger32 profite de la boucle d'événements Win32 déjà intégrée à
  Qt (`QEventDispatcherWin32`) : plus besoin de `PeekMessage`/`DispatchMessage`
  manuel comme en Python (le script ne tournait pas dans une vraie boucle
  Win32 native).

## Fichiers de ressources

`resources/dxcc_entities.txt` et `resources/dxcc_aliases.txt` sont deux
nouveaux fichiers extraits des dictionnaires Python compilés en dur
(`core/dxcc_entities.py`, `ALIASES` dans `core/dxcc.py`), pour rester
éditables sans recompilation — comme `resources/dxcc_prefixes.txt` déjà
présent dans la version Python.
