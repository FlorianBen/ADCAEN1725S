/* ADCS simulation driver
 * Mark Rivers
 * Feb. 28, 2016
 */

#include "asynNDArrayDriver.h"
#include <epicsEvent.h>
#include <epicsTime.h>

#include "CAENComm.h"
#include "CAENDigitizer.h"
#include "CAENDigitizerType.h"

#define DRIVER_VERSION 0
#define DRIVER_REVISION 0
#define DRIVER_MODIFICATION 1

#define CaenAcquireString "CN_ACQUIRE"

// Board info related
#define CaenModelNameString "CN_MODEL_NAME"
#define CaenModelString "CN_MODEL"
#define CaenChannelsString "CN_CHANNELS"
#define CaenFormFactorString "CN_FORM_FACTOR"
#define CaenFamilyCodeString "CN_FAMILY_CODE"
#define CaenAMCVersionString "CN_AMC_VERSION"
#define CaenPCBRevisionString "CN_PCB_REVISION"
#define CaenADCNBitsString "CN_N_BITS"

// IRQ related
#define IRQEnableString "CN_IRQ_ENABLE"
#define IRQLevelString "CN_IRQ_LEVEL"
#define IRQStatusIDString "CN_IRQ_STATUS_ID"
#define IRQEventNumberString "CN_IRQ_EVENT_NUMBER"
#define IRQModeString "CN_IRQ_MODES"

// Data related
#define ClearDataString "CN_CLEAR_DATA"
#define DisableEventAlignedReadoutString "CN_DISABLE_EVENT_ALIGNED_READOUT"

// Status more here
#define ReadTemperatureString "CN_READ_TEMPERATURE"

// Trigger and IO related
#define SendSWTriggerString "CN_SEND_SWTRIGGER"
#define SWTriggerModeString "CN_SWTRIGGER_MODES"
#define ExtTriggerModeString "CN_EXTTRIGGER_MODES"
#define ChannelSelfTriggerString "CN_CHANNEL_SELFTRIGGER"
#define GroupSelfTriggerString "CN_GROUP_SELFTRIGGER"
#define ChannelGroupMaskString "CN_CHANNEL_GROUPMASK"
#define ChannelTriggerThresholdString "CN_CHANNEL_TRIGGER_THRESHOLD"
#define GroupTriggerThresholdString "CN_GROUP_TRIGGER_THRESHOLD"
#define RunSynchronizationModeString "CN_RUN_SYNCHRONIZATION_MODE"
#define IOLevelString "CN_IO_LEVEL"
#define TriggerPolarityString "CN_TRIGGER_POLARITY"
#define AcquisitionModeString "CN_ACQ_MODE"

// Acquisition related
#define ChannelEnableMaskString "CN_CHANNEL_ENABLE_MASK" 
#define DPPAcquisitionModeString "CN_DPP_ACQ_MODE"
#define DPPAcquisitionParamString "CN_DPP_ACQ_PARAM"
#define DPPTriggerModeString "CN_DPP_TRIGGERMODE"

// DPP
#define DPPPreTriggerString "CN_DPP_PRE_TRIGGER"
#define DPPChannelPulsePolarityString "CN_DPP_CHANNEL_PULSE_POLARITY"


// Transfer
#define DPPEventAggregationThresString "CN_DPP_EVENT_AGGREGATION_THRES"
#define DPPEventAggregationMaxString "CN_DPP_EVENT_AGGREGATION_MAX"
#define DPPNumEventAggregateString "CN_DPP_NUM_EVENT_AGGREGATE"
#define DPPNumAggregateBLTString "CN_DPP_NUM_AGGREGATE_BLT"

// Channel related
#define DPPPSDThresholdHoldString "CN_DPP_PSD_THRHO"
#define DPPPSDThresholdString "CN_DPP_PSD_THR"
#define DPPPSDSelfThresholdString "CN_DPP_PSD_SELFT"
#define DPPPSDChargeSensString "CN_DPP_PSD_CSENS"
#define DPPPSDShortGateString "CN_DPP_PSD_SGATE"
#define DPPPSDLongGateString "CN_DPP_PSD_LGATE"
#define DPPPSDGateOffsetString "CN_DPP_PSD_PGATE"
#define DPPPSDTriggerValidationWindowString "CN_DPP_PSD_TVAW"
#define DPPPSDNumberSampleString "CN_DPP_PSD_NSBL"
#define DPPPSDDiscriminatorString "CN_DPP_PSD_DISCR"
#define DPPPSDCFDFractionString "CN_DPP_PSD_CFDF"
#define DPPPSDCFDDelayString "CN_DPP_PSD_CFDD"
#define DPPPSDPileUpString "CN_DPP_PSD_PUR"
#define DPPPSDPileUpGapString "CN_DPP_PSD_PURGAP"

// Result
#define DPPPSDRateEvString "CN_DPP_PSD_RATE_EV"
#define DPPPSDChargeLongString "CN_DPP_PSD_CHARGE_LONG"
#define DPPPSDChargeShortString "CN_DPP_PSD_CHARGE_SHORT"
#define DPPPSDValString "CN_DPP_PSD_PSD"
#define DPPPSDTimeString "CN_DPP_PSD_TIME_NS"


#define MAX_ENUM_STRING_SIZE 26
typedef struct {
  int value;
  char string[MAX_ENUM_STRING_SIZE];
} enumStruct_t;

#define MAX_SIGNALS 8

#define MAXNBITS 16

#define NUM_TRIGGER_MODES 4

static const char *triggerModeStrings[NUM_TRIGGER_MODES] = {
    "Disable", "Ext. out only", "Acq only", "Acq and Ext. out"};

/** ADC simulation driver; does 1-D waveforms on 8 channels.
 * Inherits from asynNDArrayDriver */

class ADCAEN1725S : public asynNDArrayDriver {
public:
  ADCAEN1725S(const char *portName, int LinkType, int LinkNum, int ConetNode,
              int VMEBaseAddress, int traceMask, NDDataType_t dataType, int maxBuffers,
              int memoryChannel, size_t maxMemory, int priority, int stackSize);

  /* These are the methods that we override from asynNDArrayDriver */
  virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
  virtual asynStatus readEnum(asynUser *pasynUser, char *strings[],
                              int values[], int severities[], size_t nElements,
                              size_t *nIn);
  virtual void report(FILE *fp, int details);
  void simTask(); /**< Should be private, but gets called from C, so must be
                     public */
  void dataGrabTask();
  void statusTask();

  void shutdown();

protected:
  int CN_Acquire;
#define FIRST_CN_DETECTOR_PARAM CN_Acquire
  int CN_ModelName;
  int CN_Model;
  int CN_Channels;
  int CN_FormFactor;
  int CN_FamilyCode;
  int CN_AMCVersion;
  int CN_PCB_Revision;
  int CN_ADC_NBits;

  int CN_IRQEnable;
  int CN_IRQLevel;
  int CN_IRQStatusID;
  int CN_IRQEventNumber;
  int CN_IRQMode;

  int CN_ClearData;
  int CN_DisableEventAlignedReadout;

  int CN_ReadTemperature;

  int CN_AcquisitionMode;

  int CN_ChannelEnableMask;

  int CN_SendSWTrigger;
  int CN_SWTriggerMode;
  int CN_ExtTriggerMode;
  int CN_ChannelSelfTrigger;
  int CN_GroupSelfTrigger;
  int CN_ChannelGroupMask;
  int CN_ChannelTriggerThreshold;
  int CN_GroupTriggerThreshold;
  int CN_RunSynchronizationMode;
  int CN_IOLevel;
  int CN_TriggerPolarity;

  int CN_DPPPreTrigger;
  int CN_DPPChannelPulsePolarity;
  int CN_DPPEventAggregationThres;
  int CN_DPPEventAggregationMax;
  int CN_DPPNumEventAggregate;
  int CN_DPPNumAggregateBLT;
  int CN_DPPAcquisitionMode;
  int CN_DPPAcquisitionParam;
  int CN_DPPTriggerMode;

  int CN_DPPPSDThresholdHold;
  int CN_DPPPSDThreshold;
  int CN_DPPPSDSelfThreshold;
  int CN_DPPPSDChargeSens;
  int CN_DPPPSDShortGate;
  int CN_DPPPSDLongGate;
  int CN_DPPPSDGateOffset;
  int CN_DPPPSDTriggerValidationWindow;
  int CN_DPPPSDNumberSample;
  int CN_DPPPSDDiscriminator;
  int CN_DPPPSDCFDFraction;
  int CN_DPPPSDCFDDelay;
  int CN_DPPPSDPileUp;
  int CN_DPPPSDPileUpGap;

  int CN_DPPPSDChargeLong;
  int CN_DPPPSDChargeShort;
  int CN_DPPPSDRateEv;

private:
  /* These are the methods that are new to this class */
  asynStatus createStaticEnums();
  asynStatus createDynamicEnums();

  asynStatus connectADC();
  asynStatus disconnectADC();

  asynStatus grabData();
  asynStatus startCapture();
  asynStatus stopCapture();

  asynStatus setIRQ();

  asynStatus setChannelEnableMask();

  asynStatus setDigitizerParameter();
  asynStatus setChannelParameter();
  asynStatus allocBuffer();

  // asynStatus setChannelParameter();

  /* Our data */
  CAEN_DGTZ_BoardInfo_t boardInfo_;
  CAEN_DGTZ_ConnectionType linkType_;
  int linkNum_;
  int conetNode_;
  uint32_t VMEBaseAddress_;
  int handle_;

  epicsEventId startEventId_;
  epicsEventId stopEventId_;
  int uniqueId_;
  int acquiring_;
  double elapsedTime_;

  int numValidSWTriggerModes_;
  int numValidExtTriggerModes_;

  char *buffer = NULL;                             // readout buffer
  CAEN_DGTZ_DPP_PSD_Event_t *events[MAX_SIGNALS]; // events buffer
  CAEN_DGTZ_DPP_PSD_Waveforms_t *waveform = NULL;  // waveforms buffer
  uint32_t allocatedSize, bufferSize;
  uint32_t numEvents[MAX_SIGNALS];


  enumStruct_t swtriggerModeEnums_[NUM_TRIGGER_MODES];
  enumStruct_t exttriggerModeEnums_[NUM_TRIGGER_MODES];

  NDArray *pRaw_;
};
