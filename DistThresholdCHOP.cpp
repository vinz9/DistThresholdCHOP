#include "DistThresholdCHOP.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <cmath>


double GetSeconds()
{
	static LARGE_INTEGER lastTime;
	static LARGE_INTEGER freq;
	static bool first = true;


	if (first)
	{
		QueryPerformanceCounter(&lastTime);
		QueryPerformanceFrequency(&freq);

		first = false;
	}

	static double time = 0.0;

	LARGE_INTEGER t;
	QueryPerformanceCounter(&t);

	__int64 delta = t.QuadPart - lastTime.QuadPart;
	double deltaSeconds = double(delta) / double(freq.QuadPart);

	time += deltaSeconds;

	lastTime = t;

	return time;

}

// These functions are basic C function, which the DLL loader can find
// much easier than finding a C++ Class.
// The DLLEXPORT prefix is needed so the compile exports these functions from the .dll
// you are creating
extern "C"
{

DLLEXPORT
void
FillCHOPPluginInfo(CHOP_PluginInfo *info)
{
	// Always set this to CHOPCPlusPlusAPIVersion.
	info->apiVersion = CHOPCPlusPlusAPIVersion;

	// The opType is the unique name for this CHOP. It must start with a 
	// capital A-Z character, and all the following characters must lower case
	// or numbers (a-z, 0-9)
	info->customOPInfo.opType->setString("Distthreshold");

	// The opLabel is the text that will show up in the OP Create Dialog
	info->customOPInfo.opLabel->setString("Dist Threshold");

	// Information about the author of this OP
	info->customOPInfo.authorName->setString("Vincent Houze");
	//info->customOPInfo.authorEmail->setString("email@email.com");

	info->customOPInfo.minInputs = 2;

	// It can accept up to 1 input though, which changes it's behavior
	info->customOPInfo.maxInputs = 2;
}

DLLEXPORT
CHOP_CPlusPlusBase*
CreateCHOPInstance(const OP_NodeInfo* info)
{
	// Return a new instance of your class every time this is called.
	// It will be called once per CHOP that is using the .dll
	return new DistThresholdCHOP(info);
}

DLLEXPORT
void
DestroyCHOPInstance(CHOP_CPlusPlusBase* instance)
{
	// Delete the instance here, this will be called when
	// Touch is shutting down, when the CHOP using that instance is deleted, or
	// if the CHOP loads a different DLL
	delete (DistThresholdCHOP*)instance;
}

};


DistThresholdCHOP::DistThresholdCHOP(const OP_NodeInfo *info) : myNodeInfo(info)
{
	myExecuteCount = 0;
	timer1 = 0;
	timer2 = 0;


}

DistThresholdCHOP::~DistThresholdCHOP()
{

}

void
DistThresholdCHOP::getGeneralInfo(CHOP_GeneralInfo* ginfo, const OP_Inputs* inputs, void* reserved1)
{
	// This will cause the node to cook every frame
	ginfo->cookEveryFrameIfAsked = true;
	//ginfo->timeslice = true;
	ginfo->timeslice = false;
	ginfo->inputMatchIndex = 0;
}

bool
DistThresholdCHOP::getOutputInfo(CHOP_OutputInfo* info, const OP_Inputs* inputs, void* reserved1)
{


	int maxLines = inputs->getParInt("Maxlines");
	int maxLinesPerPoint = inputs->getParInt("Maxlinesperpoint");
	float distMax = inputs->getParDouble("Distmax");

	info->numChannels = 7;

	linepos = (float**)malloc(info->numChannels*sizeof(float*));
	for (int i = 0; i < info->numChannels; i++) {
		linepos[i] = (float*)malloc(maxLines*sizeof(float));
	}

	l = 0;

	float t1 = GetSeconds();

	if (inputs->getNumInputs() > 1 && inputs->getInputCHOP(0)->numChannels>0){

		const OP_CHOPInput* chopInput0 = inputs->getInputCHOP(0);


		for (int i = 0; i < chopInput0->numSamples; i++){
				
			if(l<maxLines){

				float p1[] = { chopInput0->getChannelData(0)[i],
								chopInput0->getChannelData(1)[i],
								chopInput0->getChannelData(2)[i] };
				
					int k=0;

					const OP_CHOPInput* chopInput1 = inputs->getInputCHOP(1);

					for (int j = 0; j < chopInput1->numSamples; j++){
				
							if(l<maxLines && k<maxLinesPerPoint){

								float p2[] = { chopInput1->getChannelData(0)[j],
									chopInput1->getChannelData(1)[j],
									chopInput1->getChannelData(2)[j] };

								float sqrdist = std::pow(p2[0]-p1[0],2) + std::pow(p2[1]-p1[1],2) + std::pow(p2[2]-p1[2],2);

									//float fade = info->inputArrays->floatInputs[0].values[1];
									if (sqrdist<distMax) {
							
										linepos[0][l] = p1[0];
										linepos[1][l] = p1[1];
										linepos[2][l] = p1[2];
										linepos[3][l] = p2[0];
										linepos[4][l] = p2[1];
										linepos[5][l] = p2[2];
										//linepos[5][l] = sqrdist;
										linepos[6][l] = sqrdist;
										l++;
										k++;
									}
							}
					
						
					}
				
			}
		}


	} else {
		for (int i = 0 ; i < info->numChannels; i++){
				free(linepos[i]);
		}
	}


	if(l<1)
		l=1;


	info->numSamples = l;

	float t2 = GetSeconds();
	timer2 = t2 - t1;

	return true;
}

void
DistThresholdCHOP::getChannelName(int32_t index, OP_String *name, const OP_Inputs* inputs, void* reserved1)
{

	switch(index) {
		case 0:
			name->setString("tx1");
			break;
		case 1:
			name->setString("ty1");
			break;
		case 2:
			name->setString("tz1");
			break;
		case 3:
			name->setString("tx2");
			break;
		case 4:
			name->setString("ty2");
			break;
		case 5:
			name->setString("tz2");
			break;
		case 6:
			name->setString("sqrdist");
			break;
	}
}

void
DistThresholdCHOP::execute(CHOP_Output* output, const OP_Inputs* inputs, void* reserved)
{
	myExecuteCount++;

	float t1 = GetSeconds();

	if (inputs->getNumInputs() > 1 && inputs->getInputCHOP(0)->numChannels>0)
	{
		for (int i = 0 ; i < output->numChannels; i++)
		{
			memcpy(output->channels[i], linepos[i], l*sizeof(float));
			free(linepos[i]);
		}
	}

	float t2 = GetSeconds();
	timer1 = t2 - t1;
}

int32_t
DistThresholdCHOP::getNumInfoCHOPChans(void * reserved1)
{
	// We return the number of channel we want to output to any Info CHOP
	// connected to the CHOP. In this example we are just going to send one channel.
	return 3;
}

void
DistThresholdCHOP::getInfoCHOPChan(int32_t index, OP_InfoCHOPChan* chan, void* reserved1)
{
	// This function will be called once for each channel we said we'd want to return
	// In this example it'll only be called once.


	switch (index){

	case 0:
		chan->name->setString("executeCount");
		chan->value = myExecuteCount;
		break;

	case 1:
		chan->name->setString("timer1");
		chan->value = (float)timer1*1000;
		break;

	case 2:
		chan->name->setString("timer2");
		chan->value = (float)timer2*1000;
		break;

	}


}

bool		
DistThresholdCHOP::getInfoDATSize(OP_InfoDATSize* infoSize, void* reserved1)
{
	infoSize->rows = 1;
	infoSize->cols = 2;
	// Setting this to false means we'll be assigning values to the table
	// one row at a time. True means we'll do it one column at a time.
	infoSize->byColumn = false;
	return true;
}

void
DistThresholdCHOP::getInfoDATEntries(int32_t index, int32_t nEntries, OP_InfoDATEntries* entries, void* reserved1)
{
	if (index == 0)
	{

		static char tempBuffer2[4096];
		// Set the value for the first column
		entries->values[0]->setString("executeCount");

		// Set the value for the second column
		sprintf(tempBuffer2, "%d", myExecuteCount);
		entries->values[1]->setString(tempBuffer2);
	}
}

void
DistThresholdCHOP::setupParameters(OP_ParameterManager* manager, void *reserved1)
{
	//Distmax
	{
		OP_NumericParameter	np;

		np.name = "Distmax";
		np.label = "Distmax";
		np.defaultValues[0] = 1.0;

		OP_ParAppendResult res = manager->appendFloat(np);
		assert(res == OP_ParAppendResult::Success);
	}

	//Maxlines
	{
		OP_NumericParameter	np;

		np.name = "Maxlines";
		np.label = "Maxlines";
		np.defaultValues[0] = 1000.0;

		OP_ParAppendResult res = manager->appendInt(np);
		assert(res == OP_ParAppendResult::Success);
	}

	//Maxlinesperpoint
	{
		OP_NumericParameter	np;

		np.name = "Maxlinesperpoint";
		np.label = "Maxlinesperpoint";
		np.defaultValues[0] = 1000.0;

		OP_ParAppendResult res = manager->appendInt(np);
		assert(res == OP_ParAppendResult::Success);
	}


}