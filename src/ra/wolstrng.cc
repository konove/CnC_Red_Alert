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

#include "ra/config.h"

//	Menu choice for Internet game.
const char* const TXT_WOL_INTERNETBUTTON = "Internet";
//	Generic error message, though implies that blame lies with Westwood
// Online.
const char* const TXT_WOL_ERRORMESSAGE =
    config::kIsGerman ? "Unerwarteter Fehler trat bei der Kommunikation mit "
                        "Westwood Online auf."
    : config::kIsFrench
        ? "Erreur inattendue lors de la connexion  Westwood Online."
        : "Unexpected error occurred communicating with Westwood Online.";
//	Connect button on login dialog.
const char* const TXT_WOL_CONNECT = config::kIsGerman   ? "Verbinden"
                                    : config::kIsFrench ? "Se connecter"
                                                        : "Connect";
//	Title for login dialog.
const char* const TXT_WOL_LOGINDIALOG =
    config::kIsGerman   ? "Westwood-Online-Login"
    : config::kIsFrench ? "Identifiant  Westwood Online"
                        : "Westwood Online Login";
//	Appears on login dialog - user login name field.
const char* const TXT_WOL_NAME = config::kIsGerman   ? "Spitzname"
                                 : config::kIsFrench ? "Pseudo"
                                                     : "Nickname";
//	Appears on login dialog - user password field.
const char* const TXT_WOL_PASSWORD = config::kIsGerman   ? "Pa\251wort"
                                     : config::kIsFrench ? "Mot de passe"
                                                         : "Password";
//	Appears on login dialog - checkbox specifying whether nickname/password
// should be saved to disk.
const char* const TXT_WOL_SAVELOGIN = config::kIsGerman   ? "Speichern"
                                      : config::kIsFrench ? "Sauvegarder"
                                                          : "Save";
//	User hit the Escape button to cancel the logging in process.
const char* const TXT_WOL_LOGINCANCEL = config::kIsGerman ? "Login abgebrochen."
                                        : config::kIsFrench
                                            ? "Ouverture de session annule."
                                            : "Login cancelled.";
const char* const TXT_WOL_MISSINGNAME =
    config::kIsGerman   ? "Bitte geben Sie Ihren Login-Spitznamen ein."
    : config::kIsFrench ? "Veuillez entrer l'identifiant pour votre pseudo."
                        : "Please enter your login nickname.";
const char* const TXT_WOL_MISSINGPASSWORD =
    config::kIsGerman ? "Bitte geben Sie Ihr Login-Pa\251wort ein."
    : config::kIsFrench
        ? "Veuillez entrer l'identifiant pour votre mot de passe."
        : "Please enter your login password.";
const char* const TXT_WOL_CANTSAVENICK =
    config::kIsGerman   ? "Fehler beim Speichern des Spitznamens/Pa\251worts"
    : config::kIsFrench ? "Erreur lors de la sauvegarde du pseudo/mot de passe."
                        : "Error saving nickname/password.";
const char* const TXT_WOL_NICKINUSE =
    config::kIsGerman ? "Dieser Spitzname wird bereits verwendet. Bitte whlen "
                        "Sie einen anderen."
    : config::kIsFrench ? "Ce pseudo est dj utilis. Slectionnez-en un autre."
                        : "That nickname is in use. Please select another.";
const char* const TXT_WOL_BADPASS =
    config::kIsGerman   ? "Ungltiges Pa\251wort fr diesen Spitznamen"
    : config::kIsFrench ? "Mot de passe invalide pour ce pseudo."
                        : "Invalid password for this nickname.";
const char* const TXT_WOL_TIMEOUT =
    config::kIsGerman   ? "Verbindung zu Westwood Online unterbrochen"
    : config::kIsFrench ? "Expiration du temps de connexion  Westwood Online."
                        : "Connection to Westwood Online timed out.";
const char* const TXT_WOL_CONNECTING =
    config::kIsGerman   ? "Verbinde zu Westwood Online..."
    : config::kIsFrench ? "Connexion  Westwood Online..."
                        : "Connecting to Westwood Online...";
const char* const TXT_WOL_CANTCONNECT =
    config::kIsGerman
        ? "Verbindung zu Westwood Online konnte nicht hergestellt werden."
    : config::kIsFrench ? "Impossible d'tablir la connexion  Westwood Online."
                        : "Could not establish connection to Westwood Online.";
//	Appears while connecting and logging in to Westwood Online.
const char* const TXT_WOL_ATTEMPTLOGIN =
    config::kIsGerman   ? "Einloggen ... "
    : config::kIsFrench ? "Ouverture de la session en cours..."
                        : "Logging in...";
//	Appears while logging out and disconnecting from Westwood Online.
const char* const TXT_WOL_ATTEMPTLOGOUT =
    config::kIsGerman   ? "Ausloggen ..."
    : config::kIsFrench ? "Fermeture de la session en cours..."
                        : "Logging out...";
//	Appears while logging out and disconnecting from Westwood Online after
// an error has occurred.
const char* const TXT_WOL_ERRORLOGOUT =
    config::kIsGerman   ? "Verbindung zu Westwood Online beenden ..."
    : config::kIsFrench ? "Fin de connexion avec Westwood Online..."
                        : "Terminating connection with Westwood Online...";
//	Common "please wait" message.
const char* const TXT_WOL_WAIT =
    config::kIsGerman
        ? "Bitte warten ... Verbindung zu Westwood Online wird hergestellt ..."
    : config::kIsFrench
        ? "Attendez svp, en communication  Westwood Online..."
        : "Please wait... communicating with Westwood Online...";
//	Title for the top WW Online level.
const char* const TXT_WOL_TOPLEVELTITLE = "Westwood Online";
//	Title for the WW Online level where "official" chat channels are listed.
const char* const TXT_WOL_OFFICIALCHAT = config::kIsGerman ? "Offizieller Chat"
                                         : config::kIsFrench
                                             ? "Conversation officielle"
                                             : "Official Chat";
//	Title for the WW Online level where "user" (in other words, unofficial)
// chat channels are listed.
const char* const TXT_WOL_USERCHAT = config::kIsGerman ? "User-Chat"
                                     : config::kIsFrench
                                         ? "Conversation utilisateur"
                                         : "User Chat";
//	Title for the WW Online level where game channels are listed.
const char* const TXT_WOL_GAMECHANNELS = config::kIsGerman   ? "Game-Channels"
                                         : config::kIsFrench ? "Canaux de jeu"
                                                             : "Game Channels";
//	Title for the WW Online level where Red Alert game lobbies are listed.
const char* const TXT_WOL_REDALERTLOBBIES =
    config::kIsGerman   ? "Alarmstufe-Rot-Lobbies"
    : config::kIsFrench ? "Salons d'Alerte Rouge"
                        : "Red Alert Lobbies";
//	Appears briefly while a list of channels is being downloaded.
const char* const TXT_WOL_CHANNELLISTLOADING =
    config::kIsGerman   ? "... Daten werden heruntergeladen ..."
    : config::kIsFrench ? "...En cours de tlchargement..."
                        : "...downloading...";
const char* const TXT_WOL_YOURENOTINCHANNEL =
    config::kIsGerman
        ? "Sie befinden sich zur Zeit nicht in einem Chat-Channel."
    : config::kIsFrench ? "Vous n'tes pas dans un canal de conversation."
                        : "You are not currently in a chat channel.";
//	"Action" button. Causes text entered by user to show up as if they were
// performing an action, as opposed to speaking.
const char* const TXT_WOL_ACTION = "Action";
//	"Join" button. Allows user to join a channel, game, or WW Online level.
const char* const TXT_WOL_JOIN = config::kIsGerman   ? "Teilnehmen"
                                 : config::kIsFrench ? "Rejoindre"
                                                     : "Join";
const char* const TXT_WOL_CANTCREATEINCHANNEL =
    config::kIsGerman
        ? "Sie knnen keinen neuen Channel erstellen, bevor Sie diesen Channel "
          "verlassen."
    : config::kIsFrench
        ? "Cration d'un nouveau canal impossible tant que vous ne quittez pas "
          "ce "
          "canal."
        : "You can't create a new channel until you exit this channel.";
//	"New" button. Allows user to create a new chat channel or game.
const char* const TXT_WOL_NEWSOMETHING = config::kIsGerman   ? "Neu"
                                         : config::kIsFrench ? "Nouveau"
                                                             : "New";
//	Title for chat channel creation dialog.
const char* const TXT_WOL_CREATECHANNELTITLE =
    config::kIsGerman   ? "Channel erstellen"
    : config::kIsFrench ? "Crer un canal"
                        : "Create Channel";
const char* const TXT_WOL_CREATECHANNELPROMPT =
    config::kIsGerman   ? "Channel-Name: "
    : config::kIsFrench ? "Nom du canal : "
                        : "Channel Name: ";
//	Prompt for fields where the user must enter a password.
const char* const TXT_WOL_PASSPROMPT = config::kIsGerman   ? "Pa\251wort: "
                                       : config::kIsFrench ? "Mot de passe : "
                                                           : "Password: ";
//	Prompt for fields where the user may enter a password, but it is not
// required.
const char* const TXT_WOL_OPTIONALPASSPROMPT =
    config::kIsGerman   ? "Pa\251wort (optional): "
    : config::kIsFrench ? "Mot de passe (en option): "
                        : "Password (optional): ";
//	Appears in channel list, as top choice, which the user can use to go
// back to the top WW Online level.
const char* const TXT_WOL_CHANNEL_TOP =
    config::kIsGerman   ? ".. <zurck zum Anfang>"
    : config::kIsFrench ? ".. <retour  la page d'accueil>"
                        : ".. <back to top>";
//	Appears in channel list, as top choice, which the user can use to go
// back up one WW Online level.
const char* const TXT_WOL_CHANNEL_BACK = config::kIsGerman   ? ".. <zurck>"
                                         : config::kIsFrench ? ".. <retour>"
                                                             : ".. <back>";
//	%s is replaced by the name of a channel.
const char* const TXT_WOL_YOUJOINED =
    config::kIsGerman   ? "Sie nehmen am %s-Channel teil."
    : config::kIsFrench ? "Vous avez rejoint le canal %s."
                        : "You have joined the %s channel.";
//	%s is replaced by the name of a user.
const char* const TXT_WOL_YOUJOINEDGAME =
    config::kIsGerman   ? "Sie nehmen an %ss Spiel teil."
    : config::kIsFrench ? "Vous rejoignez la partie de %s."
                        : "You have joined %s's game.";
//	Message confirming that user created a new game.
const char* const TXT_WOL_YOUCREATEDGAME =
    config::kIsGerman   ? "Neues Spiel erstellt."
    : config::kIsFrench ? "Cration d'une nouvelle partie."
                        : "New game created.";
//	%s is replaced by the name of a lobby.
const char* const TXT_WOL_YOUJOINEDLOBBY =
    config::kIsGerman   ? "Sie haben die %s-Lobby betreten."
    : config::kIsFrench ? "Vous tes entr dans le salon %s."
                        : "You have entered the %s lobby.";
//	%s is replaced by the name of a channel.
const char* const TXT_WOL_YOULEFT =
    config::kIsGerman   ? "Sie haben den %s-Channel verlassen."
    : config::kIsFrench ? "Vous avez quitt le canal %s."
                        : "You have left the %s channel.";
//	%s is replaced by the name of a lobby.
const char* const TXT_WOL_YOULEFTLOBBY =
    config::kIsGerman   ? "Sie haben die %s-Lobby verlassen."
    : config::kIsFrench ? "Vous avez quitt le salon %s."
                        : "You have left the %s lobby.";
//	Title for dialog that prompts user for the password needed to enter a
// private channel.
const char* const TXT_WOL_JOINPRIVATETITLE =
    config::kIsGerman   ? "An privatem Channel teilnehmen"
    : config::kIsFrench ? "Rejoindre un canal priv"
                        : "Join Private Channel";
const char* const TXT_WOL_JOINPRIVATEPROMPT =
    config::kIsGerman   ? "Channel-Pa\251wort eingeben: "
    : config::kIsFrench ? "Entrer le mot de passe du canal : "
                        : "Enter Channel Password: ";
const char* const TXT_WOL_BADCHANKEY =
    config::kIsGerman   ? "Falsches Channel-Pa\251wort."
    : config::kIsFrench ? "Mot de passe du canal incorrect."
                        : "Incorrect channel password.";
//	Title for the Page/Locate dialog. Page = send a user a message. Locate =
// find out where a user is.
const char* const TXT_WOL_PAGELOCATE = config::kIsGerman ? "Senden/Suchen"
                                       : config::kIsFrench
                                           ? "Envoyer/Rechercher"
                                           : "Page/Locate";
//	Appears on Page/Locate dialog.
const char* const TXT_WOL_USERNAMEPROMPT = config::kIsGerman ? "User-Name: "
                                           : config::kIsFrench
                                               ? "Nom de l'utilisateur : "
                                               : "User Name: ";
//	Text for Page button on dialog.
const char* const TXT_WOL_PAGE = config::kIsGerman   ? "Senden"
                                 : config::kIsFrench ? "Envoyer"
                                                     : "Page";
//	Text for Locate button on dialog.
const char* const TXT_WOL_LOCATE = config::kIsGerman   ? "Suchen"
                                   : config::kIsFrench ? "Rechercher"
                                                       : "Locate";
//	%s is replaced with name of user being located.
const char* const TXT_WOL_LOCATING = config::kIsGerman ? "Suche %s..."
                                     : config::kIsFrench
                                         ? "Recherche de %s en cours ..."
                                         : "Locating %s...";
const char* const TXT_WOL_FIND_NOTHERE =
    config::kIsGerman   ? "Der gesuchte User-Name existiert nicht."
    : config::kIsFrench ? "Le nom de l'utilisateur spcifi n'existe pas."
                        : "The specified user name does not exist.";
const char* const TXT_WOL_FIND_NOCHAN =
    config::kIsGerman
        ? "Der genannte User befindet sich zur Zeit nicht in einem Channel."
    : config::kIsFrench
        ? "L'utilisateur spcifi n'est pas sur le canal pour le moment."
        : "The specified user is currently not in a channel.";
const char* const TXT_WOL_FIND_OFF =
    config::kIsGerman ? "Der genannte User hat die Suchfunktion ausgeschaltet."
    : config::kIsFrench
        ? "L'utilisateur spcifi a dsactiv la fonction de recherche."
        : "The specified user has disabled find capability.";
//	%s is replaced with name of user being located.
const char* const TXT_WOL_FOUNDIN =
    config::kIsGerman   ? "User wurde im %s-Channel gefunden."
    : config::kIsFrench ? "Utilisateur trouv dans le canal %s."
                        : "User found in the %s channel.";
//	Title for Page dialog.
const char* const TXT_WOL_PAGEMESSAGETITLE = config::kIsGerman ? "Sender"
                                             : config::kIsFrench
                                                 ? "Envoyer  l'utilisateur"
                                                 : "Page User";
//	Prompt for field in which user enters the message that is to be sent to
// user.
const char* const TXT_WOL_PAGEMESSAGEPROMPT =
    config::kIsGerman   ? "Zu sendende Nachricht: "
    : config::kIsFrench ? "Message  envoyer : "
                        : "Message to Send: ";
//	%s is replaced with name of user being paged.
const char* const TXT_WOL_PAGING = config::kIsGerman   ? "Sende an %s ..."
                                   : config::kIsFrench ? "Envoi  %s en cours..."
                                                       : "Paging %s...";
const char* const TXT_WOL_PAGE_NOTHERE =
    config::kIsGerman   ? "Der genannte User ist nicht eingeloggt."
    : config::kIsFrench ? "L'utilisateur spcifi n'a pas ouvert la session."
                        : "The specified user is not logged in.";
const char* const TXT_WOL_PAGE_OFF =
    config::kIsGerman
        ? "Der genannte User hat die Empfangsfunktion ausgeschaltet."
    : config::kIsFrench
        ? "L'utilisateur spcifi a dsactiv la fonction d'envoi de messages."
        : "The specified user has disabled page capability.";
//	First %s is replaced with user name, second %s with a text message.
const char* const TXT_WOL_ONPAGE = config::kIsGerman   ? "Sende von %s: %s"
                                   : config::kIsFrench ? "Envoi de %s : %s"
                                                       : "Page from %s: %s";
//	%s is replaced with name of user being paged.
const char* const TXT_WOL_WASPAGED =
    config::kIsGerman   ? "Die Nachricht wurde %s erfolgreich zugestellt."
    : config::kIsFrench ? "Envoi  %s russi."
                        : "%s was successfully paged.";
//	%s is replaced with the name of a user that has just been squelched.
//(Currently unused.) const char TXT_WOL_USERISSQUELCHED[]		= "%s
// has been squelched."; 	%s is replaced with the name of a user that has
// had
// squelch removed. (Currently unused.) const char TXT_WOL_USERISNOTSQUELCHED[]
// = "%s is no longer squelched.";
const char* const TXT_WOL_ONLYOWNERCANKICK =
    config::kIsGerman
        ? "Nur der Channel-Besitzer kann andere User hinauswerfen."
    : config::kIsFrench
        ? "Seul le responsable du canal peut expulser des utilisateurs."
        : "Only the channel owner can kick users out.";
//	Both %s replaced with user names.
const char* const TXT_WOL_USERKICKEDUSER =
    config::kIsGerman   ? "%s hat %s aus dem Channel geworfen."
    : config::kIsFrench ? "%s expulse %s du canal."
                        : "%s kicked %s out of the channel.";
//	%s replaced with user name.
const char* const TXT_WOL_USERKICKEDYOU =
    config::kIsGerman   ? "Sie wurden von %s aus dem Channel geworfen."
    : config::kIsFrench ? "Vous tes expuls du canal par %s."
                        : "You were kicked out of the channel by %s.";
const char* const TXT_WOL_NOONETOKICK =
    config::kIsGerman ? "Whlen Sie den/die User, die Sie hinauswerfen mchten."
    : config::kIsFrench
        ? "Slectionnez l'(les) utilisateur(s) que vous voulez expulser."
        : "Select the user(s) you wish to kick out.";
//	%s replaced with user name.
const char* const TXT_WOL_USERWASBANNED =
    config::kIsGerman   ? "%s hat keinen Zutritt mehr zu diesem Channel."
    : config::kIsFrench ? "%s est exclu du canal."
                        : "%s has been banned from the channel.";
//	Title for dialog in which user enters password for new game they are
// creating.
const char* const TXT_WOL_CREATEPRIVGAMETITLE =
    config::kIsGerman   ? "Privates Spiel erstellen"
    : config::kIsFrench ? "Crer une partie prive"
                        : "Create Private Game";
const char* const TXT_WOL_YOUREBANNED =
    config::kIsGerman   ? "Sie haben keinen Zutritt mehr zu diesem Channel."
    : config::kIsFrench ? "Vous n'tes pas autoris  entrer dans ce canal."
                        : "You've been banned from entering this channel.";
//	%s replaced with user name.
const char* const TXT_WOL_PLAYERLEFTGAME =
    config::kIsGerman   ? "%s hat das Spiel verlassen."
    : config::kIsFrench ? "%s a quitt la partie."
                        : "%s has left the game.";
//	%s replaced with user name.
const char* const TXT_WOL_PLAYERJOINEDGAME =
    config::kIsGerman   ? "%s hat an dem Spiel teilgenommen."
    : config::kIsFrench ? "%s a rejoint la partie."
                        : "%s has joined the game.";
const char* const TXT_WOL_YOUWEREKICKEDFROMGAME =
    config::kIsGerman   ? "Sie wurden aus dem Spiel geworfen."
    : config::kIsFrench ? "Vous avez t expuls de la partie."
                        : "You've been kicked out of the game.";
//	Shows user's ladder ranking and win/loss record. Appears above main chat
// area.
const char* const TXT_WOL_PERSONALWINLOSSRECORD =
    config::kIsGerman
        ? "%s. Alarmstufe Rot: Pl %u. Siege %u. Niederl %u. Pkte %u."
    : config::kIsFrench
        ? "%s. Alerte Rouge: position %u. Vict. %u. Df. %u. Pts. %u."
        : "%s. Red Alert: Ranked %u. Won %u. Lost %u. Points %u.";
//	Shows user's ladder ranking and win/loss record. Appears above main chat
// area. Appended Aftermath ranking.
const char* const TXT_WOL_PERSONALWINLOSSRECORDAM =
    config::kIsGerman
        ? "%s. Vergeltungsschlag: Pl %u. Siege %u. Niederl %u. Pkte %u."
    : config::kIsFrench
        ? "%s. Missions M.A.D.: position %u. Vict. %u. Df. %u. Pts. %u."
        : "%s. Aftermath: Ranked %u. Won %u. Lost %u. Points %u.";
//	Used to show brief user ladder ranking in user lists. Example: FredX
//(Rank 134)
const char* const TXT_WOL_USERRANK = config::kIsGerman   ? "%s (Platz %u)"
                                     : config::kIsFrench ? "%s (Position %u)"
                                                         : "%s (Rank %u)";
//	No need to translate.
const char* const TXT_WOL_USERHOUSE = "%s <%s>";
//	"Rank" translates the same here as above.
const char* const TXT_WOL_USERRANKHOUSE =
    config::kIsGerman   ? "%s (Platz %u) <%s>"
    : config::kIsFrench ? "%s (Position %u) <%s>"
                        : "%s (Rank %u) <%s>";
//	Button host user presses to start a game they have created.
const char* const TXT_WOL_STARTBUTTON = config::kIsFrench ? "Dmarrer" : "Start";
//	Button that guests joining a game press to indicate that they agree to
// the game rules set up by the host.
const char* const TXT_WOL_ACCEPTBUTTON = config::kIsGerman   ? "Besttigen"
                                         : config::kIsFrench ? "Accepter"
                                                             : "Accept";
//	%s replaced with user name.
const char* const TXT_WOL_HOSTLEFTGAME =
    config::kIsGerman   ? "%s hat das Spiel abgebrochen."
    : config::kIsFrench ? "%s a annul la partie."
                        : "%s has cancelled the game.";
//	Appears when game is actually being started.
const char* const TXT_WOL_WAITINGTOSTART =
    config::kIsGerman   ? "Spiel wird gestartet ..."
    : config::kIsFrench ? "Lancement de la partie..."
                        : "Launching game...";
//	Tooltip help for WW Online button: disconnect.
const char* const TXT_WOL_TTIP_DISCON =
    config::kIsGerman   ? " Westwood Online verlassen"
    : config::kIsFrench ? " Quitter Westwood Online "
                        : " Leave Westwood Online ";
//	Tooltip help for WW Online button: leave current channel.
const char* const TXT_WOL_TTIP_LEAVE =
    config::kIsGerman   ? " Derzeitigen Channel verlassen "
    : config::kIsFrench ? " Quitter le canal o vous vous trouvez "
                        : " Leave the channel you are in ";
//	Tooltip help for WW Online button: refresh current list.
const char* const TXT_WOL_TTIP_REFRESH =
    config::kIsGerman   ? " Channel-Liste aktualisieren "
    : config::kIsFrench ? " Rafrachir la liste du canal "
                        : " Refresh current channel list ";
//	Tooltip help for WW Online button: squelch user(s).
const char* const TXT_WOL_TTIP_SQUELCH =
    config::kIsGerman   ? " Nachrichteneingang von User(n) ein/ausschalten"
    : config::kIsFrench ? " Activer/dsactiver les messages en provenance de(s) "
                          "l'utilisateur(s) "
                        : " Enable/disable incoming message from user(s) ";
//	Tooltip help for WW Online button: ban (and kick) user(s).
const char* const TXT_WOL_TTIP_BAN =
    config::kIsGerman   ? " User(n) Zutritt zum Channel verwehren "
    : config::kIsFrench ? " Exclure l'/les utilisateur(s)du canal "
                        : " Ban user(s) from channel ";
//	Tooltip help for WW Online button: kick user(s).
const char* const TXT_WOL_TTIP_KICK =
    config::kIsGerman   ? " User aus dem Channel werfen "
    : config::kIsFrench ? " Expulser l'/les utilisateurs du canal "
                        : " Kick user(s) out of channel ";
//	Tooltip help for WW Online button: find/page.
const char* const TXT_WOL_TTIP_FINDPAGE =
    config::kIsGerman   ? " User suchen oder an User senden "
    : config::kIsFrench ? " Rechercher ou envoyer un message  un utilisateur "
                        : " Find or page a user ";
//	Tooltip help for WW Online button: show options dialog.
const char* const TXT_WOL_TTIP_OPTIONS =
    config::kIsGerman   ? " Westwood-Online-Optionen einstellen "
    : config::kIsFrench ? " Rgler les options de Westwood Online "
                        : " Set Westwood Online options ";
//	Tooltip help for WW Online button: browse game ladder.
const char* const TXT_WOL_TTIP_LADDER =
    config::kIsGerman   ? " Alarmstufe-Rot-Tabelle durchsuchen "
    : config::kIsFrench ? " Parcourir les hirarchies d'Alerte Rouge "
                        : " Browse Red Alert ladders ";
//	Tooltip help for WW Online button: show help.
const char* const TXT_WOL_TTIP_HELP =
    config::kIsGerman   ? " Westwood-Online-Hilfe anzeigen "
    : config::kIsFrench ? " Afficher l'aide de Westwood Online "
                        : " Show Westwood Online help ";
//	Tooltip help. Appears for button host presses to start a game.
const char* const TXT_WOL_TTIP_START = config::kIsGerman   ? " Spiel starten "
                                       : config::kIsFrench ? " Dmarrer le jeu "
                                                           : " Start the game ";
//	Tooltip help. Appears for button guests press in order to agree to
//(accept) game rules set up by the host.
const char* const TXT_WOL_TTIP_ACCEPT =
    config::kIsGerman   ? " Aktuelle Spieleinstellungen besttigen "
    : config::kIsFrench ? " Valider les paramtres actuels du jeu "
                        : " Accept the current game settings ";
//	Tooltip help. Appears for the small buttons that allow users to enlarge
// or diminish the size of channel/user lists.
const char* const TXT_WOL_TTIP_EXPANDLIST =
    config::kIsGerman   ? " Listen erweitern/verkleinern "
    : config::kIsFrench ? " Complter/rduire la liste "
                        : " Expand/contract list ";
//	Tooltip for Cancel button during game setup.
const char* const TXT_WOL_TTIP_CANCELGAME =
    config::kIsGerman   ? " Einen Level zurck "
    : config::kIsFrench ? " Retour au niveau prcdent "
                        : " Go back a level ";
//	Tooltip for Join button during chat.
const char* const TXT_WOL_TTIP_JOIN =
    config::kIsGerman   ? " An Chat- oder Game-Channel teilnehmen "
    : config::kIsFrench ? " Rejoindre un canal de conversation/jeu "
                        : " Join a chat or game channel ";
//	Tooltip for Back button during chat.
const char* const TXT_WOL_TTIP_BACK = config::kIsGerman ? " Einen Level zurck "
                                      : config::kIsFrench
                                          ? " Retour au niveau prcdent "
                                          : " Go back a level ";
//	Tooltip for New button during chat.
const char* const TXT_WOL_TTIP_CREATE =
    config::kIsGerman   ? " Neuen Chat- oder Game-Level erstellen "
    : config::kIsFrench ? " Crer un nouveau canal de conversation/jeu "
                        : " Create a new chat/game channel ";
//	Tooltip for Action button.
const char* const TXT_WOL_TTIP_ACTION = config::kIsGerman ? " Action-Nachricht "
                                        : config::kIsFrench
                                            ? " Message d'action "
                                            : " Action message ";
const char* const TXT_WOL_OPTFIND =
    config::kIsGerman   ? "Lassen Sie zu, da\251 andere Sie FINDEN."
    : config::kIsFrench ? "Laisser les autres vous RECHERCHER."
                        : "Let others FIND you.";
const char* const TXT_WOL_OPTPAGE =
    config::kIsGerman ? "Lassen Sie zu, da\251 andere Ihnen Nachrichten SENDEN."
    : config::kIsFrench ? "Laisser les autres vous ENVOYER des messages."
                        : "Let others PAGE you.";
const char* const TXT_WOL_OPTLANGUAGE =
    config::kIsGerman   ? "Unangemessene Sprache herausfiltern."
    : config::kIsFrench ? "Filtrer les vulgarits."
                        : "Filter out bad language.";
//	"Display just the games that were created by someone in the lobby you
// are currently in."
const char* const TXT_WOL_OPTGAMESCOPE =
    config::kIsGerman   ? "Nur lokale Spiel-Lobby anzeigen."
    : config::kIsFrench ? "Afficher seulement les parties en salons locaux."
                        : "Show local lobby games only.";
const char* const TXT_WOL_CHANNELGONE =
    config::kIsGerman   ? "Channel existiert nicht mehr."
    : config::kIsFrench ? "Ce canal n'existe plus."
                        : "Channel no longer exists.";
//	Title for create new game dialog.
const char* const TXT_WOL_CG_TITLE = config::kIsGerman   ? "Spiel erstellen"
                                     : config::kIsFrench ? "Crer une partie"
                                                         : "Create Game";
//	%i replaced by number of players allowed into game channel.
const char* const TXT_WOL_CG_PLAYERS = config::kIsGerman   ? "Spieler:  %i"
                                       : config::kIsFrench ? "Joueurs :  %i"
                                                           : "Players:  %i";
//	Marks field indicating whether or not this is a tournament game.
const char* const TXT_WOL_CG_TOURNAMENT = config::kIsGerman   ? "Turnier"
                                          : config::kIsFrench ? "Tournoi"
                                                              : "Tournament";
//	Marks field indicating whether or not this is a private game.
const char* const TXT_WOL_CG_PRIVACY = config::kIsGerman   ? "Privat"
                                       : config::kIsFrench ? "Prive"
                                                           : "Private";
const char* const TXT_WOL_CG_RAGAME = config::kIsGerman ? "Alarmstufe-Rot-Spiel"
                                      : config::kIsFrench
                                          ? "Partie Alerte Rouge"
                                          : "Red Alert game";
const char* const TXT_WOL_CG_CSGAME = config::kIsGerman ? "Gegenangriff-Spiel"
                                      : config::kIsFrench
                                          ? "Partie Missions Taga"
                                          : "Counterstrike game";
const char* const TXT_WOL_CG_AMGAME =
    config::kIsGerman   ? "Vergeltungsschlag-Spiel"
    : config::kIsFrench ? "Partie Missions M.A.D."
                        : "Aftermath game";
const char* const TXT_WOL_NEEDCOUNTERSTRIKE =
    config::kIsGerman ? "Sie mssen 'Gegenangriff' installiert haben, um dieses "
                        "Spiel spielen zu "
                        "knnen."
    : config::kIsFrench
        ? "Dsol, vous devez installer Missions Taga pour jouer cette partie."
        : "Sorry, you must have Counterstrike installed to play this game.";
const char* const TXT_WOL_NEEDAFTERMATH =
    config::kIsGerman ? "Sie mssen 'Vergeltungsschlag' installiert haben, um "
                        "dieses Spiel spielen "
                        "zu knnen."
    : config::kIsFrench
        ? "Dsol, vous devez installer Missions M.A.D. pour jouer cette partie."
        : "Sorry, you must have Aftermath installed to play this game.";
//	%s = name of channel, %i = number of people in channel.
const char* const TXT_WOL_TTIP_CHANLIST_CHAT =
    config::kIsGerman
        ? " Doppelklick zur Teilnahme am '%s'-Channel (z.Z. %i User). "
    : config::kIsFrench
        ? " Double-clic pour rejoindre canal %s (%i utilisateurs). "
        : " Doubleclick to join the '%s' channel (%i current users). ";
//	%s = name of lobby, %i = number of people in channel.
const char* const TXT_WOL_TTIP_CHANLIST_LOBBY =
    config::kIsGerman
        ? " Doppelklick zur Teilnahme an der '%s'-Lobby (z.Z. %i User). "
    : config::kIsFrench
        ? " Double-clic pour rejoindre salon %s (%i utilisateurs). "
        : " Doubleclick to join the '%s' lobby (%i current users). ";
//	Appears in tooltip help for a channel list item.
const char* const TXT_WOL_TTIP_REDALERT = config::kIsGerman   ? "Alarmstufe Rot"
                                          : config::kIsFrench ? "Alerte Rouge"
                                                              : "Red Alert";
//	Appears in tooltip help for a channel list item.
const char* const TXT_WOL_TTIP_COUNTERSTRIKE =
    config::kIsGerman   ? "Gegenangriff"
    : config::kIsFrench ? "Missions Taga"
                        : "Counterstrike";
//	Appears in tooltip help for a channel list item.
const char* const TXT_WOL_TTIP_AFTERMATH =
    config::kIsGerman   ? "Vergeltungsschlag"
    : config::kIsFrench ? "Missions M.A.D."
                        : "Aftermath";
//	%s = name of user, first %i = number of players in channel, second %i =
// maximum number of players allowed.
const char* const TXT_WOL_TTIP_CHANLIST_RAGAME =
    config::kIsGerman   ? " %s-Spiel (%i Spieler von maximal %i). "
    : config::kIsFrench ? " Partie de %s (%i joueurs pour un max. de %i). "
                        : " %s game (%i players of a maximum %i). ";
//	%s = name of user, %i = number of players in channel.
const char* const TXT_WOL_TTIP_CHANLIST_GAME =
    config::kIsGerman   ? " %s-Spiel (%i Spieler). "
    : config::kIsFrench ? " Partie de %s (%i joueurs). "
                        : " %s game (%i players). ";
//	Appears in tooltip help for a channel list item.
const char* const TXT_WOL_TTIP_PRIVATEGAME = config::kIsGerman   ? "(Privat) "
                                             : config::kIsFrench ? "(Prive) "
                                                                 : "(Private) ";
//	Appears in tooltip help for a channel list item.
const char* const TXT_WOL_TTIP_TOURNAMENTGAME = config::kIsGerman ? "(Turnier) "
                                                : config::kIsFrench
                                                    ? "(Tournoi) "
                                                    : "(Tournament) ";
//	%s is a kind of game, for example, "Dune 2000".
const char* const TXT_WOL_TTIP_CHANNELTYPE_GAMESOFTYPE =
    config::kIsGerman   ? " Doppelklicken Sie, um %s-Spiele anzusehen. "
    : config::kIsFrench ? " Double-clic pour afficher les parties %s. "
                        : " Doubleclick to view %s games. ";
const char* const TXT_WOL_TOURNAMENTPLAYERLIMIT =
    config::kIsGerman ? "Turnierspiele mssen von zwei Spieler gespielt werden."
    : config::kIsFrench
        ? "Les parties en tournoi doivent rassembler deux joueurs."
        : "Tournament games must be two player games.";
//	Shows on game setup screen for private games. %s = password for game.
const char* const TXT_WOL_PRIVATEPASSWORD = config::kIsGerman ? "Pa\251wort: %s"
                                            : config::kIsFrench
                                                ? "Mot de passe : %s"
                                                : "Password: %s";
//	User cannot join game because either he or the game host has hacked the
// game.
const char* const TXT_WOL_RULESMISMATCH =
    config::kIsGerman ? "Ihr Spiel ist mit dem des Host nicht kompatibel!"
    : config::kIsFrench
        ? "Votre partie n'est pas compatible avec celle du serveur !"
        : "Your game is incompatible with the host's!";
//	Message appears when game host presses start button but slow responses
// cause an automatic cancellation of game start.
const char* const TXT_WOL_STARTTIMEOUT =
    config::kIsGerman ? "Keine Antworten von Gsten! Spielstart abgebrochen."
    : config::kIsFrench
        ? "Expiration du temps de rponse des clients ! Dmarrage du jeu annul."
        : "Timed out waiting for guest responses! Game start cancelled.";
//	Message appears for guests when automatic cancellation occurs.
const char* const TXT_WOL_STARTCANCELLED =
    config::kIsGerman   ? "Spielstart abgebrochen."
    : config::kIsFrench ? "Dmarrage du jeu annul."
                        : "Game start cancelled.";
//	Text of button on game setup screen that takes user out of the game
// channel.
const char* const TXT_WOL_CANCELGAME = config::kIsGerman   ? "Zurck"
                                       : config::kIsFrench ? "Retour"
                                                           : "Back";
const char* const TXT_WOL_PATCHQUESTION =
    config::kIsGerman ? "Ein Update-Patch wird fr Internet-Spiele bentigt. "
                        "Mchten Sie es jetzt "
                        "herunterladen?"
    : config::kIsFrench ? "Un patch mis  jour est ncessaire pour le jeu sur "
                          "Internet. Voulez-vous "
                          "le tlcharger maintenant ?"
                        : "An update patch is required for Internet play. Do "
                          "you want to download it "
                          "now?";
//	Title of patch download dialog. First %i = current file being
// downloaded, second %i = total # of files to download.
const char* const TXT_WOL_DOWNLOADING =
    config::kIsGerman   ? "Datei %i von %i herunterladen"
    : config::kIsFrench ? "Tlcharger %i fichier(s) sur %i."
                        : "Download file %i of %i";
const char* const TXT_WOL_DOWNLOADERROR =
    config::kIsGerman   ? "Ein Fehler trat beim Herunterladen der Dateien auf."
    : config::kIsFrench ? "Erreur lors du tlchargement du fichier."
                        : "An error occurred during file download.";
//	Appears on patch download dialog. First %i = current # of bytes
// downloaded, second %i = total # of bytes to download.
const char* const TXT_WOL_DOWNLOADBYTES =
    config::kIsGerman   ? "%i Bytes von %i erhalten. (%i%%%%)"
    : config::kIsFrench ? "Rception de %i octets sur %i. (%i%%%%)."
                        : "Received %i bytes out of %i. (%i%%%%)";
//	Appears on patch download dialog. First %i = number of minutes left,
// second %i = number of seconds left.
const char* const TXT_WOL_DOWNLOADTIME =
    config::kIsGerman   ? "Verbleibende Zeit: %i Min. %i Sek."
    : config::kIsFrench ? "Temps restant : %i min. %i secs."
                        : "Time Remaining: %i min. %i secs.";
//	Appended to title of patch download dialog when resuming an interrupted
// download. %s is the regular title, as above.
const char* const TXT_WOL_DOWNLOADRESUMED =
    config::kIsGerman   ? "%s (Nach Unterbrechung wiederaufgenommen.)"
    : config::kIsFrench ? "%s (reprise aprs interruption.)"
                        : "%s (Resumed after interruption.)";
const char* const TXT_WOL_DOWNLOADCONNECTING =
    config::kIsGerman   ? "Status: Verbinde ..."
    : config::kIsFrench ? "Etat : en cours de connexion..."
                        : "Status: Connecting...";
const char* const TXT_WOL_DOWNLOADLOCATING =
    config::kIsGerman   ? "Status: Suche Datei ..."
    : config::kIsFrench ? "Etat : recherche du fichier..."
                        : "Status: Locating file...";
const char* const TXT_WOL_DOWNLOADDOWNLOADING =
    config::kIsGerman   ? "Status: Lade herunter ..."
    : config::kIsFrench ? "Etat : en cours de tlchargement..."
                        : "Status: Downloading...";
const char* const TXT_WOL_DOWNLOADEXITWARNING =
    config::kIsGerman ? "Herunterladen abgeschlossen! Alarmstufe Rot wird "
                        "jetzt neugestartet, "
                        "damit das Update-Patch angewendet werden kann."
    : config::kIsFrench
        ? "Tlchargement termin ! Alerte Rouge est relanc pour que le nouveau "
          "patch soit pris en compte."
        : "Download complete! Red Alert will now restart in order to apply the "
          "update patch.";
const char* const TXT_WOL_HELPSHELL =
    config::kIsGerman
        ? "Sind Sie sicher, da\251 Sie den Internet-Browser fr die "
          "Westwood-Online-Hilfe starten mchten?"
    : config::kIsFrench ? "Voulez-vous vraiment lancer le navigateur Internet "
                          "pour obtenir l'aide "
                          "Westwood Online ?"
                        : "Are you sure you want to launch the Internet "
                          "browser for Westwood Online "
                          "help?";
const char* const TXT_WOL_LADDERSHELL =
    config::kIsGerman
        ? "Sind Sie sicher, da\251 Sie den Internet-Browser fr die "
          "Alarmstufe-Rot-Tabellen starten mchten?"
    : config::kIsFrench ? "Voulez-vous vraiment lancer le navigateur Internet "
                          "pour les hirarchies "
                          "d'Alerte Rouge ?"
                        : "Are you sure you want to launch the Internet "
                          "browser for the Red Alert "
                          "ladders?";
const char* const TXT_WOL_WEBREGISTRATIONSHELL =
    config::kIsGerman
        ? "Keine gespeicherten User-Namen gefunden. Mchten Sie einen neuen "
          "User-Namen fr Westwood Online eintragen?"
    : config::kIsFrench ? "Aucun nom d'utilisateur sauvegard. Voulez-vous "
                          "enregistrer un nouveau "
                          "nom d'utilisateur pour Westwood Online ?"
                        : "No saved usernames found. Would you like to "
                          "register a new username for "
                          "Westwood Online?";
const char* const TXT_WOL_GAMEADVERTSHELL =
    config::kIsGerman ? "Sind Sie sicher, da\251 Sie den Internet-Browser fr "
                        "Informationen ber "
                        "%s starten mchten?"
    : config::kIsFrench
        ? "Voulez-vous vraiment lancer le navigateur Internet pour obtenir des "
          "informations sur %s ?"
        : "Are you sure you want to launch the Internet browser for "
          "information "
          "about %s?";
//	Appears above user list. %i = number of users in the current channel.
const char* const TXT_WOL_USERLIST = config::kIsGerman   ? "User: %i"
                                     : config::kIsFrench ? "Utilisateurs %i"
                                                         : "Users   %i";
//	Appears above user list to explain why no users are being listed:
// because the user is not currently in a chat channel.
const char* const TXT_WOL_NOUSERLIST =
    config::kIsGerman   ? "(nicht in einem Channel)"
    : config::kIsFrench ? "(absent du canal)"
                        : "(not in a channel)";
const char* const TXT_WOL_CANTCREATEHERE =
    config::kIsGerman
        ? "Um ein Spiel zu starten, mssen Sie in der Lobby Alarmstufe Rot sein."
    : config::kIsFrench
        ? "Pour commencer une partie, vous devez tre dans un salon d'Alerte "
          "Rouge."
        : "To start a game, you have to be in a Red Alert lobby.";
//	Appears inside game, when connection to WW Online is lost.
const char* const TXT_WOL_WOLAPIGONE =
    config::kIsGerman   ? "Verbindung zu Westwood Online verloren!"
    : config::kIsFrench ? "Perte de connexion avec Westwood Online !"
                        : "Connection to Westwood Online has been lost!";
//	Appears after game, when attempting to get back into WW Online.
const char* const TXT_WOL_WOLAPIREINIT =
    config::kIsGerman
        ? "Verbindung zu Westwood Online verloren. Verbinde erneut ..."
    : config::kIsFrench
        ? "Perte de connexion avec Westwood Online. Rinitialisation en cours..."
        : "Connection to Westwood Online was lost. Reinitializing...";
const char* const TXT_WOL_NOTPAGED =
    config::kIsGerman
        ? "Kann keine Antwort senden, niemand hat Ihnen geschrieben."
    : config::kIsFrench
        ? "Impossible de rpondre au message ; personne ne vous en a envoy."
        : "Can't respond to page; no one has paged you.";
//	Appears briefly in the space for scenario name, in game setup dialog.
const char* const TXT_WOL_SCENARIONAMEWAIT =
    config::kIsGerman   ? "Warte auf Szenario ..."
    : config::kIsFrench ? "En attente du scnario..."
                        : "waiting for scenario...";
//	Text of button on chat screen that takes user out of a chat channel, or
// up one WW Online level.
const char* const TXT_WOL_BACK = config::kIsGerman   ? "Zurck"
                                 : config::kIsFrench ? "Retour"
                                                     : "Back";
const char* const TXT_WOL_AMDISCNEEDED =
    config::kIsGerman ? "Die CD 'Vergeltungsschlag' wird fr dieses Spiel "
                        "bentigt, bitte legen "
                        "Sie sie jetzt ein."
    : config::kIsFrench ? "Le CD de Missions M.A.D. est ncessaire pour cette "
                          "partie ; insrez-le "
                          "maintenant."
                        : "The Aftermath disk will be required for this game; "
                          "please insert it now.";
const char* const TXT_WOL_CONFIRMLOGOUT =
    config::kIsGerman
        ? "Sind Sie sicher, da\251 Sie Westwood Online verlassen mchten?"
    : config::kIsFrench ? "Voulez-vous vraiment quitter Westwood Online ?"
                        : "Are you sure you want to leave Westwood Online?";
//	"Propose a stalemate" button.
const char* const TXT_WOL_PROPOSE_DRAW =
    config::kIsGerman   ? "Unentschieden vorschlagen"
    : config::kIsFrench ? "Proposer une fin avec galit"
                        : "Propose a Draw";
//	Withdraw proposed stalemate button.
const char* const TXT_WOL_RETRACT_DRAW =
    config::kIsGerman   ? "Unentschieden-Vorschlag zurckziehen"
    : config::kIsFrench ? "Annuler la proposition de fin avec galit"
                        : "Retract Draw Proposal";
//	Accept offered stalemate button.
const char* const TXT_WOL_ACCEPT_DRAW =
    config::kIsGerman   ? "Unentschieden-Vorschlag akzeptieren"
    : config::kIsFrench ? "Accepter la proposition de fin avec galit"
                        : "Accept Proposed Draw";
//	User proposes that the game be declared a stalemate.
const char* const TXT_WOL_PROPOSE_DRAW_CONFIRM =
    config::kIsGerman
        ? "Sind Sie sicher, da\251 Sie ein Unentschieden vorschlagen mchten?"
    : config::kIsFrench ? "Voulez-vous vraiment proposer une fin avec galit ?"
                        : "Are you sure you want to propose a draw?";
//	User accepts the other's offer that the game be a tie.
const char* const TXT_WOL_ACCEPT_DRAW_CONFIRM =
    config::kIsGerman
        ? "Sind Sie sicher, da\251 Sie ein Unentschieden akzeptieren mchten?"
    : config::kIsFrench ? "Voulez-vous vraiment accepter une fin avec galit ?"
                        : "Are you sure you want to accept a draw?";
const char* const TXT_WOL_DRAW_PROPOSED_LOCAL =
    config::kIsGerman ? "Sie haben vorgeschlagen, da\251 das Spiel fr "
                        "unentschieden erklrt wird."
    : config::kIsFrench
        ? "Vous proposez de terminer la partie sans vainqueur ni perdant."
        : "You have proposed that the game be declared a draw.";
const char* const TXT_WOL_DRAW_PROPOSED_OTHER =
    config::kIsGerman
        ? "%s hat vorgeschlagen, da\251 das Spiel fr unentschieden erklrt wird."
    : config::kIsFrench
        ? "%s a propos de terminer la partie sans vainqueur ni perdant."
        : "%s has proposed that the game be declared a draw.";
const char* const TXT_WOL_DRAW_RETRACTED_LOCAL =
    config::kIsGerman   ? "Sie haben Ihr Unentschieden-Angebot zurckgezogen."
    : config::kIsFrench ? "Vous avez annul votre proposition de terminer la "
                          "partie sans vainqueur "
                          "ni perdant."
                        : "You have retracted your offer of a draw.";
const char* const TXT_WOL_DRAW_RETRACTED_OTHER =
    config::kIsGerman ? "%s hat das Unentschieden-Angebot zurckgezogen."
    : config::kIsFrench
        ? "%s a annul sa proposition de terminer la partie sans vainqueur ni "
          "perdant."
        : "%s has retracted the offer of a draw.";
//	Message that appears in place of "Mission Accomplished" or "Mission
// Failed", when game is a draw.
const char* const TXT_WOL_DRAW = config::kIsGerman
                                     ? "Das Spiel ist unentschieden"
                                 : config::kIsFrench ? "Match nul"
                                                     : "The Game is a Draw";
//	Error message that appears when user's web browser can't be
// automatically launched. %s is a web site URL.
const char* const TXT_WOL_CANTLAUNCHBROWSER =
    config::kIsGerman ? "Web-Browser kann %s nicht ffnen!"
    : config::kIsFrench
        ? "Impossible de lancer le navigateur web pour ouvrir %s !"
        : "Can't launch web browser to open %s!";
const char* const TXT_WOL_CHANNELFULL =
    config::kIsGerman   ? "Dieser Chat-/Game-Channel ist voll."
    : config::kIsFrench ? "Ce canal de jeu/conversation est satur."
                        : "That chat/game channel is full.";
const char* const TXT_WOL_CHANNELTYPE_TOP =
    config::kIsGerman
        ? " Doppelklicken Sie, um zum obersten Level zu gelangen. "
    : config::kIsFrench ? " Double-clic pour retourner au premier niveau. "
                        : " Doubleclick to go to the top level. ";
const char* const TXT_WOL_CHANNELTYPE_OFFICIALCHAT =
    config::kIsGerman ? " Doppelklicken Sie, um zum offiziellen "
                        "Chat-Channel-Level zu gelangen. "
    : config::kIsFrench
        ? " Double-clic pour les canaux de conversation officiels. "
        : " Doubleclick to go to the official chat channels level. ";
const char* const TXT_WOL_CHANNELTYPE_USERCHAT =
    config::kIsGerman
        ? " Doppelklicken Sie, um zum User-Chat-Channel-Level zu gelangen. "
    : config::kIsFrench
        ? " Double-clic pour les canaux d' utilisateur. "
        : " Doubleclick to go to the user chat channels level. ";
const char* const TXT_WOL_CHANNELTYPE_GAMES =
    config::kIsGerman
        ? " Doppelklicken Sie, um zum Game-Channel-Level zu gelangen. "
    : config::kIsFrench
        ? " Double-clic pour accder au niveau des canaux de jeu. "
        : " Doubleclick to go to the game channels level. ";
const char* const TXT_WOL_CHANNELTYPE_LOADING =
    config::kIsGerman
        ? " Liste von Westwood Online wird geladen, bitte warten... "
    : config::kIsFrench ? " Chargement de la liste depuis Westwood Online, "
                          "veuillez patienter..."
                        : " Loading list from Westwood Online, please wait... ";
const char* const TXT_WOL_CHANNELTYPE_LOBBIES =
    config::kIsGerman   ? " Doppelklicken Sie, um zum Lobby-Level zu gelangen. "
    : config::kIsFrench ? " Double-clic pour accder au niveau des salons. "
                        : " Doubleclick to go to the lobbies level. ";
const char* const TXT_WOL_FINDINGLOBBY =
    config::kIsGerman   ? "Verbunden - suche verfgbare Lobby..."
    : config::kIsFrench ? "Connection : recherche de salons disponibles..."
                        : "Connected - finding available lobby to enter...";
const char* const TXT_WOL_PRIVATETOMULTIPLE =
    config::kIsGerman   ? "<Privat an mehrere User>:"
    : config::kIsFrench ? "<Message personnel adress  divers utilisateurs> :"
                        : "<Private to multiple users>:";
const char* const TXT_WOL_PRIVATETO = config::kIsGerman   ? "Privat an"
                                      : config::kIsFrench ? "Message personnel "
                                                          : "Private to";
const char* const TXT_WOL_CS_MISSIONS =
    config::kIsGerman   ? "Gegenangriff-Missionen"
    : config::kIsFrench ? "Missions extraites de Missions Taga"
                        : "Counterstrike Missions";
const char* const TXT_WOL_AM_MISSIONS =
    config::kIsGerman   ? "Vergeltungsschlag-Missionen"
    : config::kIsFrench ? "Missions extraites de Missions M.A.D."
                        : "Aftermath Missions";
const char* const TXT_WOL_CANTSQUELCHSELF =
    config::kIsGerman
        ? "Sie knnen die Option zum Lesen Ihrer eigenen Nachrichten nicht "
          "ausschalten!"
    : config::kIsFrench ? "Vous ne pouvez pas dsactiver vos propres messages!"
                        : "You cannot disable viewing of your own messages!";
//	Title of the WW Online options dialog.
const char* const TXT_WOL_OPTTITLE =
    config::kIsGerman   ? "Westwood Online-Optionen"
    : config::kIsFrench ? "Options de Westwood Online"
                        : "Westwood Online Options";
const char* const TXT_WOL_SLOWUNITBUILD =
    config::kIsGerman   ? "Einheitenbau verlangsamen"
    : config::kIsFrench ? "Ralentir la Construction"
                        : "Slow Unit Build";
const char* const TXT_WOL_THEGAMEHOST = config::kIsGerman   ? "Der Spiel-Host"
                                        : config::kIsFrench ? "Le serveur"
                                                            : "The game host";
const char* const TXT_WOL_TTIP_RANKRA =
    config::kIsGerman   ? " Alarmstufe-Rot-Platz anzeigen "
    : config::kIsFrench ? " Afficher les positions d'Alerte Rouge "
                        : " Show Red Alert ladder rankings ";
const char* const TXT_WOL_TTIP_RANKAM =
    config::kIsGerman   ? " Vergeltungsschlag-Platz anzeigen "
    : config::kIsFrench ? " Afficher les positions de Missions M.A.D. "
                        : " Show Aftermath ladder rankings ";
const char* const TXT_WOL_OPTRANKAM =
    config::kIsGerman   ? "Vergeltungsschlag-Platz anzeigen."
    : config::kIsFrench ? "Afficher les positions de Missions M.A.D."
                        : "Show Aftermath rankings (instead of Red Alert)";
const char* const TXT_WOL_CANCELMEANSFORFEIT =
    config::kIsGerman   ? " (UND BSSEN SIE DAS SPIEL EIN)"
    : config::kIsFrench ? " (ET RENONCER AU JEU)"
                        : " (AND FORFEIT THE GAME)";
const char* const TXT_WOL_DLLERROR_GETIE3 =
    config::kIsGerman
        ? "Ihre Version der Windows ist veraltet. Bauen Sie bitte zu den "
          "Windows "
          "SP1, aus oder installieren Sie Internet Explorer 3,0 oder hheres."
    : config::kIsFrench
        ? "Votre version des Windows est dmode. Amliorez s'il vous plait aux "
          "Windows SP1, ou installez l'Internet Explorer 3,0 ou plus haut."
        : "Your version of Windows is out of date. Please upgrade to Windows "
          "SP1, or "
          "install Internet Explorer 3.0 or higher.";
const char* const TXT_WOL_DLLERROR_CALLUS =
    config::kIsGerman ? "Ein unerwarteter Fehler ist aufgetreten. Bitte wenden "
                        "Sie sich an den "
                        "Technischen Kundendienst."
    : config::kIsFrench
        ? "Une erreur inattendue s'est produite. Veuillez contacter "
          "l'assistance "
          "technique de Electronic Arts."
        : "An unexpected error has occurred. Please contact Westwood technical "
          "support.";
const char* const TXT_WOL_PRIVATE = config::kIsGerman   ? "<privat>"
                                    : config::kIsFrench ? "<personnel>"
                                                        : "<private>";
