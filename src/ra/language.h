// Compiled-in messages for the build language: the few strings the game needs
// before, or outside of, the CONQUER.ENG string table, such as startup
// failures printed to the console and the out-of-memory dialog. The
// localized texts keep the DOS code page bytes of the original files.

#ifndef CNC_RED_ALERT_RA_LANGUAGE_H_
#define CNC_RED_ALERT_RA_LANGUAGE_H_

#include <string>

#include "absl/strings/str_format.h"
#include "ra/config.h"

struct LanguageText {
  const char* memory_error;
  const char* abort;
  const char* insufficient_disk;
  const char* no_ram;
  const char* setup_first;
  const char* no_mouse;
  const char* invalid_option;
  const char* map_error;
  const char* stop;
  const char* continue_button;
  const char* options;  // Command-line help.
};

inline constexpr LanguageText kEnglishText{
    .memory_error = "Error - out of memory.",
    .abort = "Abort",
    .insufficient_disk = "Insufficient Disk Space to run Red Alert.\n",
    .no_ram = "Insufficient RAM available.\n",
    .setup_first = "Run SETUP program first.\n",
    .no_mouse = "Red Alert is unable to detect your mouse driver.",
    .invalid_option = "Invalid option switch.\n",
    .map_error = "Map Error!",
    .stop = "Stop",
    .continue_button = "Continue",
    .options =
        "Red Alert (c) 1996, Westwood Studios\r\n"
        "Parameters:\r\n"
        "  -DESTNET  = Specify Network Number of destination system\r\n"
        "              (Syntax: DESTNETxx.xx.xx.xx)\r\n"
        "  -SOCKET   = Network Socket ID (0 - 16383)\n"
        "  -STEALTH  = Hide multiplayer names (\"Boss mode\")\r\n"
        "  -MESSAGES = Allow messages from outside this game.\r\n"
        "\r\n",
};

inline constexpr LanguageText kGermanText{
    .memory_error = "Fehler - Kein Speicher mehr.",
    .abort = "Abbrechen",
    .insufficient_disk =
        "Nicht genug Festplattenplatz für Command & Conquer:AR.\n",
    .no_ram = "Zuwenig Hauptspeicher verfügbar.\n",
    .setup_first = "Bitte erst das SETUP-Programm starten.\n",
    .no_mouse = "C&C:AR kann Ihren Maustreiber nicht finden...",
    .invalid_option = "Ungültiger Parameter.\n",
    .map_error = "Kartenfehler!",
    .stop = "Halt",
    .continue_button = "Weiter",
    .options =
        "C&C: Alarmstufe Rot (c) 1996, Westwood Studios\r\n"
        "Parameter:\r\n"
        "  -DESTNET  = Netzwerkkennung des Zielrechners festlegen\r\n"
        "              (Syntax: DESTNETxx.xx.xx.xx)\r\n"
        "  -SOCKET   = Kennung des Netzwerk-Sockets (0 - 16383)\n"
        "  -STEALTH  = Namen im Mehrspieler-Modus verstecken "
        "(\"Boss-Modus\")\r\n"
        "  -MESSAGES = Mitteilungen von ausserhalb des Spiels zulassen\r\n"
        "\r\n",
};

inline constexpr LanguageText kFrenchText{
    .memory_error = "Erreur - Plus de mémoire.",
    .abort = "Interrompre",
    .insufficient_disk =
        "Espace disque insuffisant pour lancer Command & Conquer.\n",
    .no_ram = "Mémoire vive (RAM) insuffisante.\n",
    .setup_first = "Lancez d'abord le programme de configuration SETUP.\n",
    .no_mouse =
        "Alerte Rouge ne peut pas détecter votre gestionnaire de souris.",
    .invalid_option = "Commande d'option invalide.\n",
    .map_error = "Erreur de carte!",
    .stop = "Stop",
    .continue_button = "Continuer",
    .options =
        "Alerte Rouge (c) 1996, Westwood Studios\r\n"
        "ParamÞtres:\r\n"
        "  -DESTNET  = Spécifier le numéro de réseau du système de "
        "destination\r\n"
        "              (Syntaxe: DESTNETxx.xx.xx.xx)\r\n"
        "  -SOCKET   = ID Socket réseau (0  16383)\r\n"
        "  -STEALTH  = Cacher les noms en mode multijoueurs (\"Mode Boss\")\r\n"
        "  -MESSAGES = Autorise les messages extérieurs à ce jeu.\r\n"
        "\r\n",
};

// The texts for this build's language.
inline constexpr const LanguageText& kLanguageText =
    config::kIsGerman   ? kGermanText
    : config::kIsFrench ? kFrenchText
                        : kEnglishText;

// "You must have %d megabytes of free disk space." in the build language.
inline std::string MustHaveDiskSpaceText(int megabytes) {
  if constexpr (config::kIsGerman) {
    return absl::StrFormat(
        "Sie brauchen %d MByte freien Platz auf der Festplatte.", megabytes);
  } else if constexpr (config::kIsFrench) {
    return absl::StrFormat(
        "Vous devez disposer de %d Mo d'espace disponsible sur le disque dur.",
        megabytes);
  } else {
    return absl::StrFormat("You must have %d megabytes of free disk space.",
                           megabytes);
  }
}

#endif  // CNC_RED_ALERT_RA_LANGUAGE_H_
