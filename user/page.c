#include "page.h"

char * page_content= "HTTP/1.0 200 OK\nContent-Type:text/html\n\n<html><head><title>Kodi-rfid</title></head><form method=\"POST\"  enctype=\"multipart/form-data\" ><center>Currently connected to <a href=\"http://SERVER:PORT\">http://SERVER:PORT</a> via ESSID<hr/><table><tr><td>Security check</td><td><input type=\"password\" name=\"security\" /></td></tr><tr><td>wifi name</td><td><input type=\"text\" name=\"essid\" /></td></tr><tr><td>wifi password</td><td><input type=\"password\" name=\"password\" /></td></tr><tr><td>Server</td><td>http://<input type=\"text\" name=\"server\" >:<input type=\"number\" name=\"port\" maxlength=\"4\" /></td></tr><tr><td></td><td><input type=\"submit\"><input name=\"upgrade\" type=\"submit\" value=\"upgrade\"></td></tr></table></form><a href=\"https://github.com/mehdilauters/kodi-rfid\" target=\"_blank\" ><img src=\"https://raw.githubusercontent.com/mehdilauters/kodi-rfid/master/doc/picture.png\" alt=\"picture\" style=\"max-width:200px;\"/><br/>kodi-rfid</a></center>DATE_BUILD</html>\n";
