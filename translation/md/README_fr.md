# CeroClient

<p align="center">
  CeroClient est un client Minecraft gratuit conçu pour être extrêmement optimisé et léger.</p>

## Fonctionnalités & Tâches

- [ ] **Instances**
	- [ ] Installer un loader (ex. Forge, Fabric, etc.)
	- [ ] Sauvegarder et gérer les instances
- [x] **Jouer à Minecraft**
	- [x] Télécharger le Manifeste
	- [x] Lire les Métadonnées
	- [x] Télécharger les Librairies
	- [x] Télécharger le Client
	- [x] Télécharger les Assets
	- [x] Démarrer le Client
	- [ ] Installer Fabric
	- [ ] Installer Forge
	- [ ] Démarrer Fabric
	- [ ] Démarrer Forge
- [x] **Connexion au compte Microsoft**
- [x] Créer un Installeur
- [x] Créer un Updater
- [ ] Réécrire le Launcher en C/C++
- [ ] Système d'amis et de messagerie
    - [x] Envoyer un message
    - [ ] Inviter dans son monde
    - [x] Ajouter un ami
    - [x] Supprimer un ami
- [ ] Optimiser le jeu
    - [ ] Réécrire toutes les versions de Minecraft en C++
        - [ ] Support de modding en Lua
    - [ ] Passer sur Vulkan
        - [ ] Support de Shader
- [ ] Ajouter un Support Android
- [ ] Support Multi Langue

---

## Installation

Tous les téléchargements et instructions pour CeroClient sont disponibles sur notre [Site Web](https://cerostudio.fr/ceroclient).

## Compilation depuis les sources

Si vous souhaitez compiler CeroClient vous-même, vous pouvez utiliser les scripts de build fournis dans le dépôt.

**Prérequis (Linux) :**
* **GCC / G++** ≥ `13.3.0`
* **Python** > 3.9 (Testé : 3.13)
* **Rust / Cargo** (Dernière version stable)
* **pkg-config**
* **Dépendances :** `gtk+-3.0`, `webkit2gtk-4.1`, `libcurl` (et `ayatana-appindicator3-0.1` ou `appindicator3-0.1` pour le support de l'icône dans la barre des tâches)

**Prérequis (Windows) :**
* **MinGW-w64** (GCC / G++)
* **Rust / Cargo** (Dernière version stable)
* Dépendances pré-compilées dans `%USERPROFILE%\mingw-deps\x64-windows`

### Instructions

1. Clonez le dépôt.
2. Installez les dépendances : `pip install -r requirements.txt`
3. Exécutez le script de build : `python3 build.py`

*Note : Les scripts de build téléchargeront automatiquement les éléments d'interface (assets) requis depuis notre CDN si l'outil de packaging interne n'est pas présent.*

---

## Licence

Ce projet est distribué sous la **PolyForm Strict License 1.0.0**. 

Le code source est disponible en lecture, à des fins d'audit et d'usage personnel. Cependant, la modification, la redistribution et l'utilisation commerciale du code sont strictement interdites. Voir le fichier [LICENSE](./LICENSE) pour plus de détails.

---

© 2025-2026 Cero Studio. Tous droits réservés.
