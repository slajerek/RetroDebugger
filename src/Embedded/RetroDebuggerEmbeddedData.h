#ifndef _RetroDebuggerEmbeddedData_H_
#define _RetroDebuggerEmbeddedData_H_

class ImFont;

ImFont* AddProFontIIx(float fontSize, int oversample);
ImFont* AddSweet16MonoFont(float fontSize, int oversample);
ImFont* AddCousineRegularFont(float fontSize, int oversample);
ImFont* AddDroidSansFont(float fontSize, int oversample);
ImFont* AddKarlaRegularFont(float fontSize, int oversample);
ImFont* AddRobotoMediumFont(float fontSize, int oversample);

void RetroDebuggerEmbeddedAddData();

#endif
