#include "CViewC64.h"
#include "CConfigStorageHjson.h"
#include "CViewDataPlot.h"
#include "CDebugSymbols.h"
#include "CDebugMemory.h"
#include "CDebugMemoryCell.h"
#include "CDataAdapter.h"
#include "CGuiMain.h"
#include "implot.h"
#include "implot_internal.h"
#include "implot_colors_extension.h"

static inline int OrderOfMagnitude(double val) { return val == 0 ? 0 : (int)(floor(log10(fabs(val)))); }
static inline int OrderToPrecision(int order) { return order > 0 ? 0 : 1 - order; }
static inline int Precision(double val) { return OrderToPrecision(OrderOfMagnitude(val)); }
static inline int AxisPrecision(const ImPlotAxis& axis) {
	const double range = axis.Ticker.TickCount() > 1 ? (axis.Ticker.Ticks[1].PlotPos - axis.Ticker.Ticks[0].PlotPos) : axis.Range.Size();
	return Precision(range);
}
static inline double RoundTo(double val, int prec) { double p = pow(10,(double)prec); return floor(val*p+0.5)/p; }
static inline double RoundAxisValue(const ImPlotAxis& axis, double value) {
	return RoundTo(value, AxisPrecision(axis));
}

CViewDataPlot::CViewDataPlot(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY,
							 CDebugSymbols *symbols, CDataAdapter *dataAdapter)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
//	imGuiNoWindowPadding = true;
//	imGuiNoScrollbar = true;
	
	this->symbols = symbols;
	this->dataAdapter = dataAdapter;
	
	dataIndexes = NULL;
	dataBuffer = NULL;
	
	char *buf = SYS_GetCharBuf();
	sprintf(buf, "%s##CViewDataPlotDataFormat", name);
	settingNameStrDataPlotFormat = STRALLOC(buf);
	SYS_ReleaseCharBuf(buf);

	dataPlotFormat = DataPlotFormat_U8;
	viewC64->config->GetInt(settingNameStrDataPlotFormat, (int*)&dataPlotFormat, (int)DataPlotFormat_U8);

	SetPlotDataLimits(0, dataAdapter->AdapterGetDataLength());
}

CViewDataPlot::~CViewDataPlot()
{
}

// TODO: data plot format
int ImPlotHex8Formatter(double value, char* buff, int size, void* data)
{
	CViewDataPlot *viewDataPlot = (CViewDataPlot*)data;
	ImGuiIO& io = ImGui::GetIO();
	ImPlotContext& gp = *GImPlot;
	ImPlotPlot &plot      = *gp.CurrentPlot;
	ImPlotAxis& x_axis = plot.XAxis(0);

	double vAxis = x_axis.PixelsToPlot(io.MousePos.x);
//	LOGD("vAxis=%f", vAxis);

	double vrAxis = vAxis; //RoundAxisValue(x_axis, vAxis);
	int index = (int)vrAxis;
	if (viewDataPlot->dataPlotFormat == DataPlotFormat_S16_LE || viewDataPlot->dataPlotFormat == DataPlotFormat_U16_LE
		|| viewDataPlot->dataPlotFormat == DataPlotFormat_S16_BE || viewDataPlot->dataPlotFormat == DataPlotFormat_U16_BE)
	{
		index = index << 1;	// *2
	}
	// note, this is not needed, as the data in buffer was already expanded
//	else if (viewDataPlot->dataPlotFormat == DataPlotFormat_U4_LE || viewDataPlot->dataPlotFormat == DataPlotFormat_U4_BE)
//	{
//		index = index >> 1; // /2
//	}

	if (viewDataPlot->dataPlotFormat == DataPlotFormat_U4_BE || viewDataPlot->dataPlotFormat == DataPlotFormat_U4_LE)
	{
		if (index < viewDataPlot->dataIndexMin || index > (viewDataPlot->dataIndexMin + viewDataPlot->dataLen*2))
		{
			buff[0] = 0;
			return 0;
		}
	}
	else
	{
		if (index < viewDataPlot->dataIndexMin || index > viewDataPlot->dataIndexMax)
		{
			buff[0] = 0;
			return 0;
		}
	}
		
	switch(viewDataPlot->dataPlotFormat)
	{
		case DataPlotFormat_U4_LE:
		case DataPlotFormat_U4_BE:
		{
			u8 v = (u8)viewDataPlot->dataBuffer[index];
			return snprintf(buff, size, "%1X", v);
		}
		case DataPlotFormat_U8:
		{
			u8 v = (u8)viewDataPlot->dataBuffer[index];
			return snprintf(buff, size, "%02X", v);
		}
		case DataPlotFormat_S8:
		{
			i8 v = (i8)viewDataPlot->dataBuffer[index];
			if (v < 0)
				return snprintf(buff, size, "-%02X", abs(v));
			return snprintf(buff, size, "%02X", v);
		}
			
		// note, data in dataBuffer is already converted to little-endian
		case DataPlotFormat_U16_LE:
		case DataPlotFormat_U16_BE:
		{
			u16 v = (u16) ((viewDataPlot->dataBuffer[index+1] << 8) | viewDataPlot->dataBuffer[index]);
			return snprintf(buff, size, "%04X", v);
		}
		case DataPlotFormat_S16_LE:
		case DataPlotFormat_S16_BE:
		{
			i16 v = (i16) ((viewDataPlot->dataBuffer[index+1] << 8) | viewDataPlot->dataBuffer[index]);
			if (v < 0)
			{
				return snprintf(buff, size, "-%04X", abs(v));
			}
			return snprintf(buff, size, "%04X", v);
		}
	}

//	LOGD("index=%d", index);
	buff[0] = 0;
	return 0;
}

int ImPlotHex16Formatter(double value, char* buff, int size, void* data)
{
	CViewDataPlot *viewDataPlot = (CViewDataPlot*)data;
	ImGuiIO& io = ImGui::GetIO();
	ImPlotContext& gp = *GImPlot;
	ImPlotPlot &plot      = *gp.CurrentPlot;
	ImPlotAxis& x_axis = plot.XAxis(0);

	double vAxis = x_axis.PixelsToPlot(io.MousePos.x);
//	LOGD("vAxis=%f", vAxis);

	double vrAxis = vAxis; //RoundAxisValue(x_axis, vAxis);
	int index = (int)vrAxis;
	if (viewDataPlot->dataPlotFormat == DataPlotFormat_S16_LE || viewDataPlot->dataPlotFormat == DataPlotFormat_U16_LE
		|| viewDataPlot->dataPlotFormat == DataPlotFormat_S16_BE || viewDataPlot->dataPlotFormat == DataPlotFormat_U16_BE)
	{
		index = index << 1;	// *2
	}
	else if (viewDataPlot->dataPlotFormat == DataPlotFormat_U4_LE || viewDataPlot->dataPlotFormat == DataPlotFormat_U4_BE)
	{
		index = index >> 1; // /2
	}

	if (index < viewDataPlot->dataIndexMin || index > viewDataPlot->dataIndexMax)
	{
		buff[0] = 0;
		return 0;
	}
	return snprintf(buff, size, "%04X", index);
}

void CViewDataPlot::SetPlotDataLimits(int min, int max)
{
	dataIndexMin = min;
	dataIndexMax = max;
	dataLen = dataIndexMax - dataIndexMin;
	
	if (dataIndexes != NULL)
		delete [] dataIndexes;
	dataIndexes = new double[dataLen * 2];	// to keep 4-bit samples too
	
	int j = dataIndexMin;
	for (int i = 0; i < dataLen; i++)
	{
		dataIndexes[i] = j++;
	}

	if (dataBuffer)
		delete [] dataBuffer;
	dataBuffer = new u8[dataLen * 2];	// to keep 4-bit samples too
}

// Note, this is a crude callback from ImPlot, it is a quick hack/POC, this should be refactored to a proper Getter
CViewDataPlot *_currentViewDataPlot = NULL;
ImU32 ImPlotColorsExtensionGetterCallback(int index)
{
	if (_currentViewDataPlot->dataPlotFormat == DataPlotFormat_S16_LE || _currentViewDataPlot->dataPlotFormat == DataPlotFormat_U16_LE
		|| _currentViewDataPlot->dataPlotFormat == DataPlotFormat_S16_BE || _currentViewDataPlot->dataPlotFormat == DataPlotFormat_U16_BE)
	{
		index = index << 1;	// *2
	}
	else if (_currentViewDataPlot->dataPlotFormat == DataPlotFormat_U4_LE || _currentViewDataPlot->dataPlotFormat == DataPlotFormat_U4_BE)
	{
		index = index >> 1; // /2
	}

	if (index < 0 || index >= _currentViewDataPlot->dataLen)
		return 0xFFFFFFFF;

	float backgroundR = 0.5f;
	float backgroundG = 0.5f;
	float backgroundB = 0.5f;
//	float backgroundR = 1.5f;

	CDebugMemoryCell *cell = _currentViewDataPlot->symbols->memory->memoryCells[index];
	float oneMinusAlpha = 1 - cell->sa;

	float newR = ((cell->sr * cell->sa) + (oneMinusAlpha * backgroundR));
	float newG = ((cell->sg * cell->sa) + (oneMinusAlpha * backgroundG));
	float newB = ((cell->sb * cell->sa) + (oneMinusAlpha * backgroundB));

//	if (cell->sr < 0.1f && cell->sg < 0.1f && cell->sb < 0.1f)
//	{
//		return ImGui::ColorConvertFloat4ToU32(ImVec4(0.7, 0.7, 0.7, 1.0));
//	}
	
//	return ImGui::ColorConvertFloat4ToU32(ImVec4(cell->sr, cell->sg, cell->sb, cell->sa));
	return ImGui::ColorConvertFloat4ToU32(ImVec4(newR, newG, newB, 1.0f));
}


void CViewDataPlot::RenderImGui()
{
	PreRenderImGui();

	// read data for plot
	dataAdapter->AdapterReadBlockDirect(dataBuffer, dataIndexMin, dataIndexMax);
	
	// macOS & Intels are little-endian
	if (dataPlotFormat == DataPlotFormat_U16_BE || dataPlotFormat == DataPlotFormat_S16_BE)
	{
		for (int i = 0; i < dataLen-1; i += 2)
		{
			u8 v0 = dataBuffer[i  ];
			u8 v1 = dataBuffer[i+1];
			dataBuffer[i  ] = v1;
			dataBuffer[i+1] = v0;
		}
	}
	else if (dataPlotFormat == DataPlotFormat_U4_LE)
	{
		int j = dataLen*2 -1;
		for (int i = dataLen-1; i >= 0; i--)
		{
			u8 v1 = (dataBuffer[i] & 0xF0) >> 4;	// 1
			u8 v0 =  dataBuffer[i] & 0x0F;			// 2

			dataBuffer[j-1] = v0;	// 2
			dataBuffer[j  ] = v1;	// 1
			
			j -= 2;
		}
	}
	else if (dataPlotFormat == DataPlotFormat_U4_BE)
	{
		int j = dataLen*2 -1;
		for (int i = dataLen-1; i >= 0; i--)
		{
			u8 v1 = (dataBuffer[i] & 0xF0) >> 4;	// 1
			u8 v0 =  dataBuffer[i] & 0x0F;			// 2

			dataBuffer[j-1] = v1;	// 1
			dataBuffer[j  ] = v0;	// 2
			
			j -= 2;
		}
	}

	//
	double xmin = 0; //dataIndexMin;
	double xmax = (double)dataLen; //dataIndexMax;
	double dataValueMin = 0;
	double dataValueMax = 255.0f;
	
	switch(dataPlotFormat)
	{
		case DataPlotFormat_U4_LE:
		case DataPlotFormat_U4_BE:
			xmax = (double)dataIndexMin + (double)dataLen*2;
			dataValueMin =   0.0f;
			dataValueMax =  15.0f;
			break;
		case DataPlotFormat_U8:
			dataValueMin =   0.0f;
			dataValueMax = 255.0f;
			break;
		case DataPlotFormat_S8:
			dataValueMin = -128.0;
			dataValueMax =  127.0;
			break;
		case DataPlotFormat_U16_LE:
		case DataPlotFormat_U16_BE:
			xmax = (double)dataIndexMin + (double)dataLen/2;
			dataValueMin =     0.0;
			dataValueMax = 65535.0;
			break;
		case DataPlotFormat_S16_LE:
		case DataPlotFormat_S16_BE:
			xmax = (double)dataIndexMin + (double)dataLen/2;
			dataValueMin = -32768.0;
			dataValueMax =  32767.0;
			break;
	}
		
	//	ImPlot::StyleColorsLight();
		
		static float weight = ImPlot::GetStyle().LineWeight;
		ImPlot::SetNextAxesLimits( xmin, xmax,
								  dataValueMin, dataValueMax, ImGuiCond_Once);
			
		float y = ImGui::GetContentRegionAvail().y;

		char *buf = SYS_GetCharBuf();
		sprintf(buf, "##%s", name);
		
		_currentViewDataPlot = this;
		
		if (ImPlot::BeginPlot(buf, ImVec2(-1, y), ImPlotFlags_Crosshairs | ImPlotFlags_NoLegend))	//ImPlotItemFlags_NoFit
		{
			ImPlot::SetupAxes("Data", "Value", ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_AutoFit|ImPlotAxisFlags_NoDecorations);
			ImPlot::SetupAxisFormat(ImAxis_X1, ImPlotHex16Formatter, this);
			ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotHex8Formatter, this);

	//		ImPlot::PushStyleVar(ImPlotStyleVar_LineWeight, weight);
	//		ImPlot::PushStyleVar(ImPlotStyleVar_Marker, ImPlotMarker_Square);
	//		ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(1, 0, 0, 1));

			switch(dataPlotFormat)
			{
				case DataPlotFormat_U4_LE:
				case DataPlotFormat_U4_BE:
					ImPlot::PlotStairsWithColorsExtension("##DataPlot", (u8 *)dataBuffer, dataLen*2);
					break;
				case DataPlotFormat_U8:
					//ImPlot::PlotStairs("##DataPlot", (u8 *)dataBuffer, dataLen);
					ImPlot::PlotStairsWithColorsExtension("##DataPlot", (u8 *)dataBuffer, dataLen);
					break;
				case DataPlotFormat_S8:
					ImPlot::PlotStairsWithColorsExtension("##DataPlot", (i8 *)dataBuffer, dataLen);
					break;
				case DataPlotFormat_U16_LE:
				case DataPlotFormat_U16_BE:
					ImPlot::PlotStairsWithColorsExtension("##DataPlot", (u16*)dataBuffer, dataLen/2);
					break;
				case DataPlotFormat_S16_LE:
				case DataPlotFormat_S16_BE:
					ImPlot::PlotStairsWithColorsExtension("##DataPlot", (i16*)dataBuffer, dataLen/2);
					break;
			}
			
	//		ImPlot::PopStyleColor();
	//		ImPlot::PopStyleVar(2);

			ImPlot::EndPlot(this);
		}
	

	SYS_ReleaseCharBuf(buf);
	
	PostRenderImGui();
}

// ImPlot takes over popup thus we need a callback to add our menu items
void CViewDataPlot::ImPlotContextMenuRenderCallback()
{
	RenderContextMenuItems();
}

// and skip engine's popup
bool CViewDataPlot::HasContextMenuItems()
{
	return false;
}

void CViewDataPlot::RenderContextMenuItems()
{
	ImGui::Separator();
	if (ImGui::BeginMenu("Data format"))
	{
		bool p = (dataPlotFormat == DataPlotFormat_U4_LE);
		if (ImGui::MenuItem(" 4-bit unsigned LE", NULL, &p))
		{
			dataPlotFormat = DataPlotFormat_U4_LE;
			viewC64->config->SetInt(settingNameStrDataPlotFormat, (int*)&dataPlotFormat);
		}
		p = (dataPlotFormat == DataPlotFormat_U4_BE);
		if (ImGui::Selectable(" 4-bit unsigned BE", (dataPlotFormat == DataPlotFormat_U4_BE)))
		{
			dataPlotFormat = DataPlotFormat_U4_BE;
			viewC64->config->SetInt(settingNameStrDataPlotFormat, (int*)&dataPlotFormat);
		}
		p = (dataPlotFormat == DataPlotFormat_U8);
		if (ImGui::Selectable(" 8-bit unsigned", (dataPlotFormat == DataPlotFormat_U8)))
		{
			dataPlotFormat = DataPlotFormat_U8;
			viewC64->config->SetInt(settingNameStrDataPlotFormat, (int*)&dataPlotFormat);
		}
		p = (dataPlotFormat == DataPlotFormat_S8);
		if (ImGui::Selectable(" 8-bit signed", (dataPlotFormat == DataPlotFormat_S8)))
		{
			dataPlotFormat = DataPlotFormat_S8;
			viewC64->config->SetInt(settingNameStrDataPlotFormat, (int*)&dataPlotFormat);
		}
		p = (dataPlotFormat == DataPlotFormat_U16_LE);
		if (ImGui::Selectable("16-bit unsigned LE", (dataPlotFormat == DataPlotFormat_U16_LE)))
		{
			dataPlotFormat = DataPlotFormat_U16_LE;
			viewC64->config->SetInt(settingNameStrDataPlotFormat, (int*)&dataPlotFormat);
		}
		p = (dataPlotFormat == DataPlotFormat_S16_LE);
		if (ImGui::Selectable("16-bit signed LE", (dataPlotFormat == DataPlotFormat_S16_LE)))
		{
			dataPlotFormat = DataPlotFormat_S16_LE;
			viewC64->config->SetInt(settingNameStrDataPlotFormat, (int*)&dataPlotFormat);
		}
		p = (dataPlotFormat == DataPlotFormat_U16_BE);
		if (ImGui::Selectable("16-bit unsigned BE", (dataPlotFormat == DataPlotFormat_U16_BE)))
		{
			dataPlotFormat = DataPlotFormat_U16_BE;
			viewC64->config->SetInt(settingNameStrDataPlotFormat, (int*)&dataPlotFormat);
		}
		p = (dataPlotFormat == DataPlotFormat_S16_BE);
		if (ImGui::Selectable("16-bit signed BE", (dataPlotFormat == DataPlotFormat_S16_BE)))
		{
			dataPlotFormat = DataPlotFormat_S16_BE;
			viewC64->config->SetInt(settingNameStrDataPlotFormat, (int*)&dataPlotFormat);
		}
		ImGui::EndMenu();
	}
}

void CViewDataPlot::ActivateView()
{
	LOGG("CViewDataPlot::ActivateView()");
}

void CViewDataPlot::DeactivateView()
{
	LOGG("CViewDataPlot::DeactivateView()");
}
