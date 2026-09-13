/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// Westwood Online strings for the build language. These never lived in
// CONQUER.ENG, so the translations are compiled in. The German and French
// texts keep the DOS code page bytes of the original files; the German
// release also replaced "\337" (sharp s) with "\251" where the 8 point font
// is used (see 8point.lbm).

#include "ra/wolstrng.h"

#include "absl/base/attributes.h"
#include "ra/config.h"

// Picks the string for the build language; English unless the build is German
// or French.
static constexpr const char* Localized(
    const char* german, const char* french,
    const char* english ABSL_ATTRIBUTE_LIFETIME_BOUND) noexcept {
  if (config::kIsGerman) {
    return german;
  }
  if (config::kIsFrench) {
    return french;
  }
  return english;
}

//	Menu choice for Internet game.
const char* const TXT_WOL_INTERNETBUTTON = "Internet";
//	Generic error message, though implies that blame lies with Westwood
// Online.
const char* const TXT_WOL_ERRORMESSAGE = Localized(
    "Unerwarteter Fehler trat bei der Kommunikation mit "
    "Westwood Online auf.",
    "Erreur inattendue lors de la connexion  Westwood Online.",
    "Unexpected error occurred communicating with Westwood Online.");
//	Connect button on login dialog.
const char* const TXT_WOL_CONNECT =
    Localized("Verbinden", "Se connecter", "Connect");
//	Title for login dialog.
const char* const TXT_WOL_LOGINDIALOG =
    Localized("Westwood-Online-Login", "Identifiant  Westwood Online",
              "Westwood Online Login");
//	Appears on login dialog - user login name field.
const char* const TXT_WOL_NAME = Localized("Spitzname", "Pseudo", "Nickname");
//	Appears on login dialog - user password field.
const char* const TXT_WOL_PASSWORD =
    Localized("Pa\251wort", "Mot de passe", "Password");
//	Appears on login dialog - checkbox specifying whether nickname/password
// should be saved to disk.
const char* const TXT_WOL_SAVELOGIN =
    Localized("Speichern", "Sauvegarder", "Save");
//	User hit the Escape button to cancel the logging in process.
const char* const TXT_WOL_LOGINCANCEL = Localized(
    "Login abgebrochen.", "Ouverture de session annule.", "Login cancelled.");
const char* const TXT_WOL_MISSINGNAME =
    Localized("Bitte geben Sie Ihren Login-Spitznamen ein.",
              "Veuillez entrer l'identifiant pour votre pseudo.",
              "Please enter your login nickname.");
const char* const TXT_WOL_MISSINGPASSWORD =
    Localized("Bitte geben Sie Ihr Login-Pa\251wort ein.",
              "Veuillez entrer l'identifiant pour votre mot de passe.",
              "Please enter your login password.");
const char* const TXT_WOL_CANTSAVENICK =
    Localized("Fehler beim Speichern des Spitznamens/Pa\251worts",
              "Erreur lors de la sauvegarde du pseudo/mot de passe.",
              "Error saving nickname/password.");
const char* const TXT_WOL_NICKINUSE = Localized(
    "Dieser Spitzname wird bereits verwendet. Bitte whlen "
    "Sie einen anderen.",
    "Ce pseudo est dj utilis. Slectionnez-en un autre.",
    "That nickname is in use. Please select another.");
const char* const TXT_WOL_BADPASS =
    Localized("Ungltiges Pa\251wort fr diesen Spitznamen",
              "Mot de passe invalide pour ce pseudo.",
              "Invalid password for this nickname.");
const char* const TXT_WOL_TIMEOUT =
    Localized("Verbindung zu Westwood Online unterbrochen",
              "Expiration du temps de connexion  Westwood Online.",
              "Connection to Westwood Online timed out.");
const char* const TXT_WOL_CONNECTING =
    Localized("Verbinde zu Westwood Online...", "Connexion  Westwood Online...",
              "Connecting to Westwood Online...");
const char* const TXT_WOL_CANTCONNECT =
    Localized("Verbindung zu Westwood Online konnte nicht hergestellt werden.",
              "Impossible d'tablir la connexion  Westwood Online.",
              "Could not establish connection to Westwood Online.");
//	Appears while connecting and logging in to Westwood Online.
const char* const TXT_WOL_ATTEMPTLOGIN = Localized(
    "Einloggen ... ", "Ouverture de la session en cours...", "Logging in...");
//	Appears while logging out and disconnecting from Westwood Online.
const char* const TXT_WOL_ATTEMPTLOGOUT = Localized(
    "Ausloggen ...", "Fermeture de la session en cours...", "Logging out...");
//	Appears while logging out and disconnecting from Westwood Online after
// an error has occurred.
const char* const TXT_WOL_ERRORLOGOUT =
    Localized("Verbindung zu Westwood Online beenden ...",
              "Fin de connexion avec Westwood Online...",
              "Terminating connection with Westwood Online...");
//	Common "please wait" message.
const char* const TXT_WOL_WAIT = Localized(
    "Bitte warten ... Verbindung zu Westwood Online wird hergestellt ...",
    "Attendez svp, en communication  Westwood Online...",
    "Please wait... communicating with Westwood Online...");
//	Title for the top WW Online level.
const char* const TXT_WOL_TOPLEVELTITLE = "Westwood Online";
//	Title for the WW Online level where "official" chat channels are listed.
const char* const TXT_WOL_OFFICIALCHAT =
    Localized("Offizieller Chat", "Conversation officielle", "Official Chat");
//	Title for the WW Online level where "user" (in other words, unofficial)
// chat channels are listed.
const char* const TXT_WOL_USERCHAT =
    Localized("User-Chat", "Conversation utilisateur", "User Chat");
//	Title for the WW Online level where game channels are listed.
const char* const TXT_WOL_GAMECHANNELS =
    Localized("Game-Channels", "Canaux de jeu", "Game Channels");
//	Title for the WW Online level where Red Alert game lobbies are listed.
const char* const TXT_WOL_REDALERTLOBBIES = Localized(
    "Alarmstufe-Rot-Lobbies", "Salons d'Alerte Rouge", "Red Alert Lobbies");
//	Appears briefly while a list of channels is being downloaded.
const char* const TXT_WOL_CHANNELLISTLOADING =
    Localized("... Daten werden heruntergeladen ...",
              "...En cours de tlchargement...", "...downloading...");
const char* const TXT_WOL_YOURENOTINCHANNEL =
    Localized("Sie befinden sich zur Zeit nicht in einem Chat-Channel.",
              "Vous n'tes pas dans un canal de conversation.",
              "You are not currently in a chat channel.");
//	"Action" button. Causes text entered by user to show up as if they were
// performing an action, as opposed to speaking.
const char* const TXT_WOL_ACTION = "Action";
//	"Join" button. Allows user to join a channel, game, or WW Online level.
const char* const TXT_WOL_JOIN = Localized("Teilnehmen", "Rejoindre", "Join");
const char* const TXT_WOL_CANTCREATEINCHANNEL = Localized(
    "Sie knnen keinen neuen Channel erstellen, bevor Sie diesen Channel "
    "verlassen.",
    "Cration d'un nouveau canal impossible tant que vous ne quittez pas "
    "ce "
    "canal.",
    "You can't create a new channel until you exit this channel.");
//	"New" button. Allows user to create a new chat channel or game.
const char* const TXT_WOL_NEWSOMETHING = Localized("Neu", "Nouveau", "New");
//	Title for chat channel creation dialog.
const char* const TXT_WOL_CREATECHANNELTITLE =
    Localized("Channel erstellen", "Crer un canal", "Create Channel");
const char* const TXT_WOL_CREATECHANNELPROMPT =
    Localized("Channel-Name: ", "Nom du canal : ", "Channel Name: ");
//	Prompt for fields where the user must enter a password.
const char* const TXT_WOL_PASSPROMPT =
    Localized("Pa\251wort: ", "Mot de passe : ", "Password: ");
//	Prompt for fields where the user may enter a password, but it is not
// required.
const char* const TXT_WOL_OPTIONALPASSPROMPT =
    Localized("Pa\251wort (optional): ", "Mot de passe (en option): ",
              "Password (optional): ");
//	Appears in channel list, as top choice, which the user can use to go
// back to the top WW Online level.
const char* const TXT_WOL_CHANNEL_TOP =
    Localized(".. <zurck zum Anfang>", ".. <retour  la page d'accueil>",
              ".. <back to top>");
//	Appears in channel list, as top choice, which the user can use to go
// back up one WW Online level.
const char* const TXT_WOL_CHANNEL_BACK =
    Localized(".. <zurck>", ".. <retour>", ".. <back>");
//	%s is replaced by the name of a channel.
const char* const TXT_WOL_YOUJOINED = Localized(
    "Sie nehmen am %s-Channel teil.", "Vous avez rejoint le canal %s.",
    "You have joined the %s channel.");
//	%s is replaced by the name of a user.
const char* const TXT_WOL_YOUJOINEDGAME =
    Localized("Sie nehmen an %ss Spiel teil.",
              "Vous rejoignez la partie de %s.", "You have joined %s's game.");
//	Message confirming that user created a new game.
const char* const TXT_WOL_YOUCREATEDGAME =
    Localized("Neues Spiel erstellt.", "Cration d'une nouvelle partie.",
              "New game created.");
//	%s is replaced by the name of a lobby.
const char* const TXT_WOL_YOUJOINEDLOBBY = Localized(
    "Sie haben die %s-Lobby betreten.", "Vous tes entr dans le salon %s.",
    "You have entered the %s lobby.");
//	%s is replaced by the name of a channel.
const char* const TXT_WOL_YOULEFT =
    Localized("Sie haben den %s-Channel verlassen.",
              "Vous avez quitt le canal %s.", "You have left the %s channel.");
//	%s is replaced by the name of a lobby.
const char* const TXT_WOL_YOULEFTLOBBY =
    Localized("Sie haben die %s-Lobby verlassen.",
              "Vous avez quitt le salon %s.", "You have left the %s lobby.");
//	Title for dialog that prompts user for the password needed to enter a
// private channel.
const char* const TXT_WOL_JOINPRIVATETITLE =
    Localized("An privatem Channel teilnehmen", "Rejoindre un canal priv",
              "Join Private Channel");
const char* const TXT_WOL_JOINPRIVATEPROMPT =
    Localized("Channel-Pa\251wort eingeben: ",
              "Entrer le mot de passe du canal : ", "Enter Channel Password: ");
const char* const TXT_WOL_BADCHANKEY = Localized(
    "Falsches Channel-Pa\251wort.", "Mot de passe du canal incorrect.",
    "Incorrect channel password.");
//	Title for the Page/Locate dialog. Page = send a user a message. Locate =
// find out where a user is.
const char* const TXT_WOL_PAGELOCATE =
    Localized("Senden/Suchen", "Envoyer/Rechercher", "Page/Locate");
//	Appears on Page/Locate dialog.
const char* const TXT_WOL_USERNAMEPROMPT =
    Localized("User-Name: ", "Nom de l'utilisateur : ", "User Name: ");
//	Text for Page button on dialog.
const char* const TXT_WOL_PAGE = Localized("Senden", "Envoyer", "Page");
//	Text for Locate button on dialog.
const char* const TXT_WOL_LOCATE = Localized("Suchen", "Rechercher", "Locate");
//	%s is replaced with name of user being located.
const char* const TXT_WOL_LOCATING =
    Localized("Suche %s...", "Recherche de %s en cours ...", "Locating %s...");
const char* const TXT_WOL_FIND_NOTHERE =
    Localized("Der gesuchte User-Name existiert nicht.",
              "Le nom de l'utilisateur spcifi n'existe pas.",
              "The specified user name does not exist.");
const char* const TXT_WOL_FIND_NOCHAN = Localized(
    "Der genannte User befindet sich zur Zeit nicht in einem Channel.",
    "L'utilisateur spcifi n'est pas sur le canal pour le moment.",
    "The specified user is currently not in a channel.");
const char* const TXT_WOL_FIND_OFF =
    Localized("Der genannte User hat die Suchfunktion ausgeschaltet.",
              "L'utilisateur spcifi a dsactiv la fonction de recherche.",
              "The specified user has disabled find capability.");
//	%s is replaced with name of user being located.
const char* const TXT_WOL_FOUNDIN = Localized(
    "User wurde im %s-Channel gefunden.", "Utilisateur trouv dans le canal %s.",
    "User found in the %s channel.");
//	Title for Page dialog.
const char* const TXT_WOL_PAGEMESSAGETITLE =
    Localized("Sender", "Envoyer  l'utilisateur", "Page User");
//	Prompt for field in which user enters the message that is to be sent to
// user.
const char* const TXT_WOL_PAGEMESSAGEPROMPT = Localized(
    "Zu sendende Nachricht: ", "Message  envoyer : ", "Message to Send: ");
//	%s is replaced with name of user being paged.
const char* const TXT_WOL_PAGING =
    Localized("Sende an %s ...", "Envoi  %s en cours...", "Paging %s...");
const char* const TXT_WOL_PAGE_NOTHERE =
    Localized("Der genannte User ist nicht eingeloggt.",
              "L'utilisateur spcifi n'a pas ouvert la session.",
              "The specified user is not logged in.");
const char* const TXT_WOL_PAGE_OFF =
    Localized("Der genannte User hat die Empfangsfunktion ausgeschaltet.",
              "L'utilisateur spcifi a dsactiv la fonction d'envoi de messages.",
              "The specified user has disabled page capability.");
//	First %s is replaced with user name, second %s with a text message.
const char* const TXT_WOL_ONPAGE =
    Localized("Sende von %s: %s", "Envoi de %s : %s", "Page from %s: %s");
//	%s is replaced with name of user being paged.
const char* const TXT_WOL_WASPAGED =
    Localized("Die Nachricht wurde %s erfolgreich zugestellt.",
              "Envoi  %s russi.", "%s was successfully paged.");
//	%s is replaced with the name of a user that has just been squelched.
//(Currently unused.) const char TXT_WOL_USERISSQUELCHED[]		= "%s
// has been squelched."; 	%s is replaced with the name of a user that has
// had
// squelch removed. (Currently unused.) const char TXT_WOL_USERISNOTSQUELCHED[]
// = "%s is no longer squelched.";
const char* const TXT_WOL_ONLYOWNERCANKICK =
    Localized("Nur der Channel-Besitzer kann andere User hinauswerfen.",
              "Seul le responsable du canal peut expulser des utilisateurs.",
              "Only the channel owner can kick users out.");
//	Both %s replaced with user names.
const char* const TXT_WOL_USERKICKEDUSER =
    Localized("%s hat %s aus dem Channel geworfen.", "%s expulse %s du canal.",
              "%s kicked %s out of the channel.");
//	%s replaced with user name.
const char* const TXT_WOL_USERKICKEDYOU =
    Localized("Sie wurden von %s aus dem Channel geworfen.",
              "Vous tes expuls du canal par %s.",
              "You were kicked out of the channel by %s.");
const char* const TXT_WOL_NOONETOKICK =
    Localized("Whlen Sie den/die User, die Sie hinauswerfen mchten.",
              "Slectionnez l'(les) utilisateur(s) que vous voulez expulser.",
              "Select the user(s) you wish to kick out.");
//	%s replaced with user name.
const char* const TXT_WOL_USERWASBANNED =
    Localized("%s hat keinen Zutritt mehr zu diesem Channel.",
              "%s est exclu du canal.", "%s has been banned from the channel.");
//	Title for dialog in which user enters password for new game they are
// creating.
const char* const TXT_WOL_CREATEPRIVGAMETITLE = Localized(
    "Privates Spiel erstellen", "Crer une partie prive", "Create Private Game");
const char* const TXT_WOL_YOUREBANNED =
    Localized("Sie haben keinen Zutritt mehr zu diesem Channel.",
              "Vous n'tes pas autoris  entrer dans ce canal.",
              "You've been banned from entering this channel.");
//	%s replaced with user name.
const char* const TXT_WOL_PLAYERLEFTGAME =
    Localized("%s hat das Spiel verlassen.", "%s a quitt la partie.",
              "%s has left the game.");
//	%s replaced with user name.
const char* const TXT_WOL_PLAYERJOINEDGAME =
    Localized("%s hat an dem Spiel teilgenommen.", "%s a rejoint la partie.",
              "%s has joined the game.");
const char* const TXT_WOL_YOUWEREKICKEDFROMGAME = Localized(
    "Sie wurden aus dem Spiel geworfen.", "Vous avez t expuls de la partie.",
    "You've been kicked out of the game.");
//	Shows user's ladder ranking and win/loss record. Appears above main chat
// area.
const char* const TXT_WOL_PERSONALWINLOSSRECORD =
    Localized("%s. Alarmstufe Rot: Pl %u. Siege %u. Niederl %u. Pkte %u.",
              "%s. Alerte Rouge: position %u. Vict. %u. Df. %u. Pts. %u.",
              "%s. Red Alert: Ranked %u. Won %u. Lost %u. Points %u.");
//	Shows user's ladder ranking and win/loss record. Appears above main chat
// area. Appended Aftermath ranking.
const char* const TXT_WOL_PERSONALWINLOSSRECORDAM =
    Localized("%s. Vergeltungsschlag: Pl %u. Siege %u. Niederl %u. Pkte %u.",
              "%s. Missions M.A.D.: position %u. Vict. %u. Df. %u. Pts. %u.",
              "%s. Aftermath: Ranked %u. Won %u. Lost %u. Points %u.");
//	Used to show brief user ladder ranking in user lists. Example: FredX
//(Rank 134)
const char* const TXT_WOL_USERRANK =
    Localized("%s (Platz %u)", "%s (Position %u)", "%s (Rank %u)");
//	No need to translate.
const char* const TXT_WOL_USERHOUSE = "%s <%s>";
//	"Rank" translates the same here as above.
const char* const TXT_WOL_USERRANKHOUSE = Localized(
    "%s (Platz %u) <%s>", "%s (Position %u) <%s>", "%s (Rank %u) <%s>");
//	Button host user presses to start a game they have created.
const char* const TXT_WOL_STARTBUTTON = config::kIsFrench ? "Dmarrer" : "Start";
//	Button that guests joining a game press to indicate that they agree to
// the game rules set up by the host.
const char* const TXT_WOL_ACCEPTBUTTON =
    Localized("Besttigen", "Accepter", "Accept");
//	%s replaced with user name.
const char* const TXT_WOL_HOSTLEFTGAME =
    Localized("%s hat das Spiel abgebrochen.", "%s a annul la partie.",
              "%s has cancelled the game.");
//	Appears when game is actually being started.
const char* const TXT_WOL_WAITINGTOSTART =
    Localized("Spiel wird gestartet ...", "Lancement de la partie...",
              "Launching game...");
//	Tooltip help for WW Online button: disconnect.
const char* const TXT_WOL_TTIP_DISCON =
    Localized(" Westwood Online verlassen", " Quitter Westwood Online ",
              " Leave Westwood Online ");
//	Tooltip help for WW Online button: leave current channel.
const char* const TXT_WOL_TTIP_LEAVE = Localized(
    " Derzeitigen Channel verlassen ", " Quitter le canal o vous vous trouvez ",
    " Leave the channel you are in ");
//	Tooltip help for WW Online button: refresh current list.
const char* const TXT_WOL_TTIP_REFRESH =
    Localized(" Channel-Liste aktualisieren ", " Rafrachir la liste du canal ",
              " Refresh current channel list ");
//	Tooltip help for WW Online button: squelch user(s).
const char* const TXT_WOL_TTIP_SQUELCH =
    Localized(" Nachrichteneingang von User(n) ein/ausschalten",
              " Activer/dsactiver les messages en provenance de(s) "
              "l'utilisateur(s) ",
              " Enable/disable incoming message from user(s) ");
//	Tooltip help for WW Online button: ban (and kick) user(s).
const char* const TXT_WOL_TTIP_BAN = Localized(
    " User(n) Zutritt zum Channel verwehren ",
    " Exclure l'/les utilisateur(s)du canal ", " Ban user(s) from channel ");
//	Tooltip help for WW Online button: kick user(s).
const char* const TXT_WOL_TTIP_KICK = Localized(
    " User aus dem Channel werfen ", " Expulser l'/les utilisateurs du canal ",
    " Kick user(s) out of channel ");
//	Tooltip help for WW Online button: find/page.
const char* const TXT_WOL_TTIP_FINDPAGE =
    Localized(" User suchen oder an User senden ",
              " Rechercher ou envoyer un message  un utilisateur ",
              " Find or page a user ");
//	Tooltip help for WW Online button: show options dialog.
const char* const TXT_WOL_TTIP_OPTIONS = Localized(
    " Westwood-Online-Optionen einstellen ",
    " Rgler les options de Westwood Online ", " Set Westwood Online options ");
//	Tooltip help for WW Online button: browse game ladder.
const char* const TXT_WOL_TTIP_LADDER = Localized(
    " Alarmstufe-Rot-Tabelle durchsuchen ",
    " Parcourir les hirarchies d'Alerte Rouge ", " Browse Red Alert ladders ");
//	Tooltip help for WW Online button: show help.
const char* const TXT_WOL_TTIP_HELP = Localized(
    " Westwood-Online-Hilfe anzeigen ", " Afficher l'aide de Westwood Online ",
    " Show Westwood Online help ");
//	Tooltip help. Appears for button host presses to start a game.
const char* const TXT_WOL_TTIP_START =
    Localized(" Spiel starten ", " Dmarrer le jeu ", " Start the game ");
//	Tooltip help. Appears for button guests press in order to agree to
//(accept) game rules set up by the host.
const char* const TXT_WOL_TTIP_ACCEPT =
    Localized(" Aktuelle Spieleinstellungen besttigen ",
              " Valider les paramtres actuels du jeu ",
              " Accept the current game settings ");
//	Tooltip help. Appears for the small buttons that allow users to enlarge
// or diminish the size of channel/user lists.
const char* const TXT_WOL_TTIP_EXPANDLIST =
    Localized(" Listen erweitern/verkleinern ", " Complter/rduire la liste ",
              " Expand/contract list ");
//	Tooltip for Cancel button during game setup.
const char* const TXT_WOL_TTIP_CANCELGAME = Localized(
    " Einen Level zurck ", " Retour au niveau prcdent ", " Go back a level ");
//	Tooltip for Join button during chat.
const char* const TXT_WOL_TTIP_JOIN =
    Localized(" An Chat- oder Game-Channel teilnehmen ",
              " Rejoindre un canal de conversation/jeu ",
              " Join a chat or game channel ");
//	Tooltip for Back button during chat.
const char* const TXT_WOL_TTIP_BACK = Localized(
    " Einen Level zurck ", " Retour au niveau prcdent ", " Go back a level ");
//	Tooltip for New button during chat.
const char* const TXT_WOL_TTIP_CREATE =
    Localized(" Neuen Chat- oder Game-Level erstellen ",
              " Crer un nouveau canal de conversation/jeu ",
              " Create a new chat/game channel ");
//	Tooltip for Action button.
const char* const TXT_WOL_TTIP_ACTION =
    Localized(" Action-Nachricht ", " Message d'action ", " Action message ");
const char* const TXT_WOL_OPTFIND =
    Localized("Lassen Sie zu, da\251 andere Sie FINDEN.",
              "Laisser les autres vous RECHERCHER.", "Let others FIND you.");
const char* const TXT_WOL_OPTPAGE = Localized(
    "Lassen Sie zu, da\251 andere Ihnen Nachrichten SENDEN.",
    "Laisser les autres vous ENVOYER des messages.", "Let others PAGE you.");
const char* const TXT_WOL_OPTLANGUAGE =
    Localized("Unangemessene Sprache herausfiltern.", "Filtrer les vulgarits.",
              "Filter out bad language.");
//	"Display just the games that were created by someone in the lobby you
// are currently in."
const char* const TXT_WOL_OPTGAMESCOPE =
    Localized("Nur lokale Spiel-Lobby anzeigen.",
              "Afficher seulement les parties en salons locaux.",
              "Show local lobby games only.");
const char* const TXT_WOL_CHANNELGONE =
    Localized("Channel existiert nicht mehr.", "Ce canal n'existe plus.",
              "Channel no longer exists.");
//	Title for create new game dialog.
const char* const TXT_WOL_CG_TITLE =
    Localized("Spiel erstellen", "Crer une partie", "Create Game");
//	%i replaced by number of players allowed into game channel.
const char* const TXT_WOL_CG_PLAYERS =
    Localized("Spieler:  %i", "Joueurs :  %i", "Players:  %i");
//	Marks field indicating whether or not this is a tournament game.
const char* const TXT_WOL_CG_TOURNAMENT =
    Localized("Turnier", "Tournoi", "Tournament");
//	Marks field indicating whether or not this is a private game.
const char* const TXT_WOL_CG_PRIVACY = Localized("Privat", "Prive", "Private");
const char* const TXT_WOL_CG_RAGAME =
    Localized("Alarmstufe-Rot-Spiel", "Partie Alerte Rouge", "Red Alert game");
const char* const TXT_WOL_CG_CSGAME = Localized(
    "Gegenangriff-Spiel", "Partie Missions Taga", "Counterstrike game");
const char* const TXT_WOL_CG_AMGAME = Localized(
    "Vergeltungsschlag-Spiel", "Partie Missions M.A.D.", "Aftermath game");
const char* const TXT_WOL_NEEDCOUNTERSTRIKE = Localized(
    "Sie mssen 'Gegenangriff' installiert haben, um dieses "
    "Spiel spielen zu "
    "knnen.",
    "Dsol, vous devez installer Missions Taga pour jouer cette partie.",
    "Sorry, you must have Counterstrike installed to play this game.");
const char* const TXT_WOL_NEEDAFTERMATH = Localized(
    "Sie mssen 'Vergeltungsschlag' installiert haben, um "
    "dieses Spiel spielen "
    "zu knnen.",
    "Dsol, vous devez installer Missions M.A.D. pour jouer cette partie.",
    "Sorry, you must have Aftermath installed to play this game.");
//	%s = name of channel, %i = number of people in channel.
const char* const TXT_WOL_TTIP_CHANLIST_CHAT =
    Localized(" Doppelklick zur Teilnahme am '%s'-Channel (z.Z. %i User). ",
              " Double-clic pour rejoindre canal %s (%i utilisateurs). ",
              " Doubleclick to join the '%s' channel (%i current users). ");
//	%s = name of lobby, %i = number of people in channel.
const char* const TXT_WOL_TTIP_CHANLIST_LOBBY =
    Localized(" Doppelklick zur Teilnahme an der '%s'-Lobby (z.Z. %i User). ",
              " Double-clic pour rejoindre salon %s (%i utilisateurs). ",
              " Doubleclick to join the '%s' lobby (%i current users). ");
//	Appears in tooltip help for a channel list item.
const char* const TXT_WOL_TTIP_REDALERT =
    Localized("Alarmstufe Rot", "Alerte Rouge", "Red Alert");
//	Appears in tooltip help for a channel list item.
const char* const TXT_WOL_TTIP_COUNTERSTRIKE =
    Localized("Gegenangriff", "Missions Taga", "Counterstrike");
//	Appears in tooltip help for a channel list item.
const char* const TXT_WOL_TTIP_AFTERMATH =
    Localized("Vergeltungsschlag", "Missions M.A.D.", "Aftermath");
//	%s = name of user, first %i = number of players in channel, second %i =
// maximum number of players allowed.
const char* const TXT_WOL_TTIP_CHANLIST_RAGAME =
    Localized(" %s-Spiel (%i Spieler von maximal %i). ",
              " Partie de %s (%i joueurs pour un max. de %i). ",
              " %s game (%i players of a maximum %i). ");
//	%s = name of user, %i = number of players in channel.
const char* const TXT_WOL_TTIP_CHANLIST_GAME =
    Localized(" %s-Spiel (%i Spieler). ", " Partie de %s (%i joueurs). ",
              " %s game (%i players). ");
//	Appears in tooltip help for a channel list item.
const char* const TXT_WOL_TTIP_PRIVATEGAME =
    Localized("(Privat) ", "(Prive) ", "(Private) ");
//	Appears in tooltip help for a channel list item.
const char* const TXT_WOL_TTIP_TOURNAMENTGAME =
    Localized("(Turnier) ", "(Tournoi) ", "(Tournament) ");
//	%s is a kind of game, for example, "Dune 2000".
const char* const TXT_WOL_TTIP_CHANNELTYPE_GAMESOFTYPE =
    Localized(" Doppelklicken Sie, um %s-Spiele anzusehen. ",
              " Double-clic pour afficher les parties %s. ",
              " Doubleclick to view %s games. ");
const char* const TXT_WOL_TOURNAMENTPLAYERLIMIT =
    Localized("Turnierspiele mssen von zwei Spieler gespielt werden.",
              "Les parties en tournoi doivent rassembler deux joueurs.",
              "Tournament games must be two player games.");
//	Shows on game setup screen for private games. %s = password for game.
const char* const TXT_WOL_PRIVATEPASSWORD =
    Localized("Pa\251wort: %s", "Mot de passe : %s", "Password: %s");
//	User cannot join game because either he or the game host has hacked the
// game.
const char* const TXT_WOL_RULESMISMATCH =
    Localized("Ihr Spiel ist mit dem des Host nicht kompatibel!",
              "Votre partie n'est pas compatible avec celle du serveur !",
              "Your game is incompatible with the host's!");
//	Message appears when game host presses start button but slow responses
// cause an automatic cancellation of game start.
const char* const TXT_WOL_STARTTIMEOUT = Localized(
    "Keine Antworten von Gsten! Spielstart abgebrochen.",
    "Expiration du temps de rponse des clients ! Dmarrage du jeu annul.",
    "Timed out waiting for guest responses! Game start cancelled.");
//	Message appears for guests when automatic cancellation occurs.
const char* const TXT_WOL_STARTCANCELLED =
    Localized("Spielstart abgebrochen.", "Dmarrage du jeu annul.",
              "Game start cancelled.");
//	Text of button on game setup screen that takes user out of the game
// channel.
const char* const TXT_WOL_CANCELGAME = Localized("Zurck", "Retour", "Back");
const char* const TXT_WOL_PATCHQUESTION = Localized(
    "Ein Update-Patch wird fr Internet-Spiele bentigt. "
    "Mchten Sie es jetzt "
    "herunterladen?",
    "Un patch mis  jour est ncessaire pour le jeu sur "
    "Internet. Voulez-vous "
    "le tlcharger maintenant ?",
    "An update patch is required for Internet play. Do "
    "you want to download it "
    "now?");
//	Title of patch download dialog. First %i = current file being
// downloaded, second %i = total # of files to download.
const char* const TXT_WOL_DOWNLOADING =
    Localized("Datei %i von %i herunterladen",
              "Tlcharger %i fichier(s) sur %i.", "Download file %i of %i");
const char* const TXT_WOL_DOWNLOADERROR =
    Localized("Ein Fehler trat beim Herunterladen der Dateien auf.",
              "Erreur lors du tlchargement du fichier.",
              "An error occurred during file download.");
//	Appears on patch download dialog. First %i = current # of bytes
// downloaded, second %i = total # of bytes to download.
const char* const TXT_WOL_DOWNLOADBYTES =
    Localized("%i Bytes von %i erhalten. (%i%%%%)",
              "Rception de %i octets sur %i. (%i%%%%).",
              "Received %i bytes out of %i. (%i%%%%)");
//	Appears on patch download dialog. First %i = number of minutes left,
// second %i = number of seconds left.
const char* const TXT_WOL_DOWNLOADTIME = Localized(
    "Verbleibende Zeit: %i Min. %i Sek.", "Temps restant : %i min. %i secs.",
    "Time Remaining: %i min. %i secs.");
//	Appended to title of patch download dialog when resuming an interrupted
// download. %s is the regular title, as above.
const char* const TXT_WOL_DOWNLOADRESUMED = Localized(
    "%s (Nach Unterbrechung wiederaufgenommen.)",
    "%s (reprise aprs interruption.)", "%s (Resumed after interruption.)");
const char* const TXT_WOL_DOWNLOADCONNECTING =
    Localized("Status: Verbinde ...", "Etat : en cours de connexion...",
              "Status: Connecting...");
const char* const TXT_WOL_DOWNLOADLOCATING =
    Localized("Status: Suche Datei ...", "Etat : recherche du fichier...",
              "Status: Locating file...");
const char* const TXT_WOL_DOWNLOADDOWNLOADING =
    Localized("Status: Lade herunter ...", "Etat : en cours de tlchargement...",
              "Status: Downloading...");
const char* const TXT_WOL_DOWNLOADEXITWARNING = Localized(
    "Herunterladen abgeschlossen! Alarmstufe Rot wird "
    "jetzt neugestartet, "
    "damit das Update-Patch angewendet werden kann.",
    "Tlchargement termin ! Alerte Rouge est relanc pour que le nouveau "
    "patch soit pris en compte.",
    "Download complete! Red Alert will now restart in order to apply the "
    "update patch.");
const char* const TXT_WOL_HELPSHELL = Localized(
    "Sind Sie sicher, da\251 Sie den Internet-Browser fr die "
    "Westwood-Online-Hilfe starten mchten?",
    "Voulez-vous vraiment lancer le navigateur Internet "
    "pour obtenir l'aide "
    "Westwood Online ?",
    "Are you sure you want to launch the Internet "
    "browser for Westwood Online "
    "help?");
const char* const TXT_WOL_LADDERSHELL = Localized(
    "Sind Sie sicher, da\251 Sie den Internet-Browser fr die "
    "Alarmstufe-Rot-Tabellen starten mchten?",
    "Voulez-vous vraiment lancer le navigateur Internet "
    "pour les hirarchies "
    "d'Alerte Rouge ?",
    "Are you sure you want to launch the Internet "
    "browser for the Red Alert "
    "ladders?");
const char* const TXT_WOL_WEBREGISTRATIONSHELL = Localized(
    "Keine gespeicherten User-Namen gefunden. Mchten Sie einen neuen "
    "User-Namen fr Westwood Online eintragen?",
    "Aucun nom d'utilisateur sauvegard. Voulez-vous "
    "enregistrer un nouveau "
    "nom d'utilisateur pour Westwood Online ?",
    "No saved usernames found. Would you like to "
    "register a new username for "
    "Westwood Online?");
const char* const TXT_WOL_GAMEADVERTSHELL = Localized(
    "Sind Sie sicher, da\251 Sie den Internet-Browser fr "
    "Informationen ber "
    "%s starten mchten?",
    "Voulez-vous vraiment lancer le navigateur Internet pour obtenir des "
    "informations sur %s ?",
    "Are you sure you want to launch the Internet browser for "
    "information "
    "about %s?");
//	Appears above user list. %i = number of users in the current channel.
const char* const TXT_WOL_USERLIST =
    Localized("User: %i", "Utilisateurs %i", "Users   %i");
//	Appears above user list to explain why no users are being listed:
// because the user is not currently in a chat channel.
const char* const TXT_WOL_NOUSERLIST = Localized(
    "(nicht in einem Channel)", "(absent du canal)", "(not in a channel)");
const char* const TXT_WOL_CANTCREATEHERE = Localized(
    "Um ein Spiel zu starten, mssen Sie in der Lobby Alarmstufe Rot sein.",
    "Pour commencer une partie, vous devez tre dans un salon d'Alerte "
    "Rouge.",
    "To start a game, you have to be in a Red Alert lobby.");
//	Appears inside game, when connection to WW Online is lost.
const char* const TXT_WOL_WOLAPIGONE =
    Localized("Verbindung zu Westwood Online verloren!",
              "Perte de connexion avec Westwood Online !",
              "Connection to Westwood Online has been lost!");
//	Appears after game, when attempting to get back into WW Online.
const char* const TXT_WOL_WOLAPIREINIT = Localized(
    "Verbindung zu Westwood Online verloren. Verbinde erneut ...",
    "Perte de connexion avec Westwood Online. Rinitialisation en cours...",
    "Connection to Westwood Online was lost. Reinitializing...");
const char* const TXT_WOL_NOTPAGED =
    Localized("Kann keine Antwort senden, niemand hat Ihnen geschrieben.",
              "Impossible de rpondre au message ; personne ne vous en a envoy.",
              "Can't respond to page; no one has paged you.");
//	Appears briefly in the space for scenario name, in game setup dialog.
const char* const TXT_WOL_SCENARIONAMEWAIT =
    Localized("Warte auf Szenario ...", "En attente du scnario...",
              "waiting for scenario...");
//	Text of button on chat screen that takes user out of a chat channel, or
// up one WW Online level.
const char* const TXT_WOL_BACK = Localized("Zurck", "Retour", "Back");
const char* const TXT_WOL_AMDISCNEEDED = Localized(
    "Die CD 'Vergeltungsschlag' wird fr dieses Spiel "
    "bentigt, bitte legen "
    "Sie sie jetzt ein.",
    "Le CD de Missions M.A.D. est ncessaire pour cette "
    "partie ; insrez-le "
    "maintenant.",
    "The Aftermath disk will be required for this game; "
    "please insert it now.");
const char* const TXT_WOL_CONFIRMLOGOUT =
    Localized("Sind Sie sicher, da\251 Sie Westwood Online verlassen mchten?",
              "Voulez-vous vraiment quitter Westwood Online ?",
              "Are you sure you want to leave Westwood Online?");
//	"Propose a stalemate" button.
const char* const TXT_WOL_PROPOSE_DRAW =
    Localized("Unentschieden vorschlagen", "Proposer une fin avec galit",
              "Propose a Draw");
//	Withdraw proposed stalemate button.
const char* const TXT_WOL_RETRACT_DRAW = Localized(
    "Unentschieden-Vorschlag zurckziehen",
    "Annuler la proposition de fin avec galit", "Retract Draw Proposal");
//	Accept offered stalemate button.
const char* const TXT_WOL_ACCEPT_DRAW = Localized(
    "Unentschieden-Vorschlag akzeptieren",
    "Accepter la proposition de fin avec galit", "Accept Proposed Draw");
//	User proposes that the game be declared a stalemate.
const char* const TXT_WOL_PROPOSE_DRAW_CONFIRM = Localized(
    "Sind Sie sicher, da\251 Sie ein Unentschieden vorschlagen mchten?",
    "Voulez-vous vraiment proposer une fin avec galit ?",
    "Are you sure you want to propose a draw?");
//	User accepts the other's offer that the game be a tie.
const char* const TXT_WOL_ACCEPT_DRAW_CONFIRM = Localized(
    "Sind Sie sicher, da\251 Sie ein Unentschieden akzeptieren mchten?",
    "Voulez-vous vraiment accepter une fin avec galit ?",
    "Are you sure you want to accept a draw?");
const char* const TXT_WOL_DRAW_PROPOSED_LOCAL = Localized(
    "Sie haben vorgeschlagen, da\251 das Spiel fr "
    "unentschieden erklrt wird.",
    "Vous proposez de terminer la partie sans vainqueur ni perdant.",
    "You have proposed that the game be declared a draw.");
const char* const TXT_WOL_DRAW_PROPOSED_OTHER = Localized(
    "%s hat vorgeschlagen, da\251 das Spiel fr unentschieden erklrt wird.",
    "%s a propos de terminer la partie sans vainqueur ni perdant.",
    "%s has proposed that the game be declared a draw.");
const char* const TXT_WOL_DRAW_RETRACTED_LOCAL =
    Localized("Sie haben Ihr Unentschieden-Angebot zurckgezogen.",
              "Vous avez annul votre proposition de terminer la "
              "partie sans vainqueur "
              "ni perdant.",
              "You have retracted your offer of a draw.");
const char* const TXT_WOL_DRAW_RETRACTED_OTHER = Localized(
    "%s hat das Unentschieden-Angebot zurckgezogen.",
    "%s a annul sa proposition de terminer la partie sans vainqueur ni "
    "perdant.",
    "%s has retracted the offer of a draw.");
//	Message that appears in place of "Mission Accomplished" or "Mission
// Failed", when game is a draw.
const char* const TXT_WOL_DRAW =
    Localized("Das Spiel ist unentschieden", "Match nul", "The Game is a Draw");
//	Error message that appears when user's web browser can't be
// automatically launched. %s is a web site URL.
const char* const TXT_WOL_CANTLAUNCHBROWSER =
    Localized("Web-Browser kann %s nicht ffnen!",
              "Impossible de lancer le navigateur web pour ouvrir %s !",
              "Can't launch web browser to open %s!");
const char* const TXT_WOL_CHANNELFULL =
    Localized("Dieser Chat-/Game-Channel ist voll.",
              "Ce canal de jeu/conversation est satur.",
              "That chat/game channel is full.");
const char* const TXT_WOL_CHANNELTYPE_TOP =
    Localized(" Doppelklicken Sie, um zum obersten Level zu gelangen. ",
              " Double-clic pour retourner au premier niveau. ",
              " Doubleclick to go to the top level. ");
const char* const TXT_WOL_CHANNELTYPE_OFFICIALCHAT = Localized(
    " Doppelklicken Sie, um zum offiziellen "
    "Chat-Channel-Level zu gelangen. ",
    " Double-clic pour les canaux de conversation officiels. ",
    " Doubleclick to go to the official chat channels level. ");
const char* const TXT_WOL_CHANNELTYPE_USERCHAT = Localized(
    " Doppelklicken Sie, um zum User-Chat-Channel-Level zu gelangen. ",
    " Double-clic pour les canaux d' utilisateur. ",
    " Doubleclick to go to the user chat channels level. ");
const char* const TXT_WOL_CHANNELTYPE_GAMES =
    Localized(" Doppelklicken Sie, um zum Game-Channel-Level zu gelangen. ",
              " Double-clic pour accder au niveau des canaux de jeu. ",
              " Doubleclick to go to the game channels level. ");
const char* const TXT_WOL_CHANNELTYPE_LOADING =
    Localized(" Liste von Westwood Online wird geladen, bitte warten... ",
              " Chargement de la liste depuis Westwood Online, "
              "veuillez patienter...",
              " Loading list from Westwood Online, please wait... ");
const char* const TXT_WOL_CHANNELTYPE_LOBBIES =
    Localized(" Doppelklicken Sie, um zum Lobby-Level zu gelangen. ",
              " Double-clic pour accder au niveau des salons. ",
              " Doubleclick to go to the lobbies level. ");
const char* const TXT_WOL_FINDINGLOBBY =
    Localized("Verbunden - suche verfgbare Lobby...",
              "Connection : recherche de salons disponibles...",
              "Connected - finding available lobby to enter...");
const char* const TXT_WOL_PRIVATETOMULTIPLE =
    Localized("<Privat an mehrere User>:",
              "<Message personnel adress  divers utilisateurs> :",
              "<Private to multiple users>:");
const char* const TXT_WOL_PRIVATETO =
    Localized("Privat an", "Message personnel ", "Private to");
const char* const TXT_WOL_CS_MISSIONS =
    Localized("Gegenangriff-Missionen", "Missions extraites de Missions Taga",
              "Counterstrike Missions");
const char* const TXT_WOL_AM_MISSIONS =
    Localized("Vergeltungsschlag-Missionen",
              "Missions extraites de Missions M.A.D.", "Aftermath Missions");
const char* const TXT_WOL_CANTSQUELCHSELF = Localized(
    "Sie knnen die Option zum Lesen Ihrer eigenen Nachrichten nicht "
    "ausschalten!",
    "Vous ne pouvez pas dsactiver vos propres messages!",
    "You cannot disable viewing of your own messages!");
//	Title of the WW Online options dialog.
const char* const TXT_WOL_OPTTITLE =
    Localized("Westwood Online-Optionen", "Options de Westwood Online",
              "Westwood Online Options");
const char* const TXT_WOL_SLOWUNITBUILD = Localized(
    "Einheitenbau verlangsamen", "Ralentir la Construction", "Slow Unit Build");
const char* const TXT_WOL_THEGAMEHOST =
    Localized("Der Spiel-Host", "Le serveur", "The game host");
const char* const TXT_WOL_TTIP_RANKRA =
    Localized(" Alarmstufe-Rot-Platz anzeigen ",
              " Afficher les positions d'Alerte Rouge ",
              " Show Red Alert ladder rankings ");
const char* const TXT_WOL_TTIP_RANKAM =
    Localized(" Vergeltungsschlag-Platz anzeigen ",
              " Afficher les positions de Missions M.A.D. ",
              " Show Aftermath ladder rankings ");
const char* const TXT_WOL_OPTRANKAM =
    Localized("Vergeltungsschlag-Platz anzeigen.",
              "Afficher les positions de Missions M.A.D.",
              "Show Aftermath rankings (instead of Red Alert)");
const char* const TXT_WOL_CANCELMEANSFORFEIT =
    Localized(" (UND BSSEN SIE DAS SPIEL EIN)", " (ET RENONCER AU JEU)",
              " (AND FORFEIT THE GAME)");
const char* const TXT_WOL_DLLERROR_GETIE3 = Localized(
    "Ihre Version der Windows ist veraltet. Bauen Sie bitte zu den "
    "Windows "
    "SP1, aus oder installieren Sie Internet Explorer 3,0 oder hheres.",
    "Votre version des Windows est dmode. Amliorez s'il vous plait aux "
    "Windows SP1, ou installez l'Internet Explorer 3,0 ou plus haut.",
    "Your version of Windows is out of date. Please upgrade to Windows "
    "SP1, or "
    "install Internet Explorer 3.0 or higher.");
const char* const TXT_WOL_DLLERROR_CALLUS = Localized(
    "Ein unerwarteter Fehler ist aufgetreten. Bitte wenden "
    "Sie sich an den "
    "Technischen Kundendienst.",
    "Une erreur inattendue s'est produite. Veuillez contacter "
    "l'assistance "
    "technique de Electronic Arts.",
    "An unexpected error has occurred. Please contact Westwood technical "
    "support.");
const char* const TXT_WOL_PRIVATE =
    Localized("<privat>", "<personnel>", "<private>");
