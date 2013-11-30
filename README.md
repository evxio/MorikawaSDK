MorikawaSDK
===========

An arduino base application programming environment for satellite's mission.<br/>
<br/>
currently supported APIs:<br/>
<br/>
static  TSTMorikawa&        getInstance             (void);<br/>
static  TSTError            getMemorySpec           (MemorySpec* result);<br/>
static  TSTError            getMemoryInfo           (MemoryInfo* result);<br/>
static  TSTError            getMemoryLog            (MemoryInfo* result);<br/>
static  void                saveMemoryLog           (void);<br/>
static  TSTError            getSelfTestLog          (SelfTestLog* result);<br/>
static  void                eraseSelfTestLog        (void);<br/>
static  unsigned long       getSizeEEPROM           (void);<br/>
static  unsigned long       getSizeSharedMemory     (void);<br/>
static  unsigned long       getSizeFRAM             (void);<br/>
static  unsigned long       getSizeFlashROM         (void);<br/>
static  unsigned int        getPageSizeSharedMemory (void);<br/>
static  unsigned int        getPageSizeFlashROM     (void);<br/>
static  unsigned long       getSectorSizeFlashROM   (void);<br/>
        unsigned long       getBootTime             (void) const;<br/>
        unsigned char       getBootCount            (void) const;<br/>
        unsigned char       getBootMode             (void) const;<br/>
        TSTError            getParamNote            (NoteParam* result);<br/>
        TSTError            getParamMorse           (MorseParam* result);<br/>
        TSTError            getParamDigitalker      (DigitalkerParam* result);<br/>
        TSTError            getParamCamera          (CameraParam* result);<br/>
        TSTError            getTelemetryTime        (TimeType type, unsigned long* result) const;<br/>
        TSTError            getTelemetryVoltage     (VoltageType type, double* result) const;<br/>
        TSTError            getTelemetryCurrent     (CurrentType type, double* result) const;<br/>
        TSTError            getTelemetryTemperature (TemperatureType type, double* result) const;<br/>
        TSTError            getTelemetryGyro        (GyroType type, double* result) const;<br/>
        TSTError            getTelemetryMagnet      (MagnetType type, double* result) const;<br/>
        TSTError            setFlashROMEraseMode    (bool param);<br/>
        bool                getFlashROMEraseMode    (void) const;<br/>
        TSTError            setText                 (TextType index, char const* text, int length = -1);<br/>
        TSTError            setTextPGM              (TextType index, char const PROGMEM* text, int length = -1);<br/>
        TSTError            getText                 (TextType index, char* text, unsigned int length, int* result = NULL);<br/>
        TSTError            setLED                  (LEDType index, unsigned char pwm);<br/>
        unsigned char       getLED                  (LEDType index) const;<br/>
        TSTError            setNoteBPM              (int param = -1);<br/>
        int                 getNoteBPM              (void) const;<br/>
        TSTError            setMorseWPM             (int param = -1);<br/>
        int                 getMorseWPM             (void) const;<br/>
        TSTError            setSpeakAsyncMode       (bool param);<br/>
        bool                getSpeakAsyncMode       (void) const;<br/>
        bool                isValid                 (void) const;<br/>
        bool                isValidSharedMemory     (void) const;<br/>
        bool                isValidFRAM             (void) const;<br/>
        bool                isValidFlashROM         (void) const;<br/>
        bool                isValidLED              (void) const;<br/>
        bool                isValidTone             (void) const;<br/>
        bool                isValidDigitalker       (void) const;<br/>
        bool                isValidCamera           (void) const;<br/>
        TSTError            isBusyDigitalker        (bool* result) const;<br/>
        bool                hasAbnormalShutdown     (void) const;<br/>
        TSTError            setup                   (void);<br/>
        void                cleanup                 (void);<br/>
static  void                shutdown                (void);<br/>
        void                loop                    (void);<br/>
static  TSTError            writeEEPROM             (unsigned long address, void const* data, unsigned int size, unsigned int* result = NULL);<br/>
static  TSTError            writeEEPROMPGM          (unsigned long address, void const PROGMEM* data, unsigned int size, unsigned int* result = NULL);<br/>
static  TSTError            readEEPROM              (unsigned long address, void* data, unsigned int size, unsigned int* result = NULL);<br/>
static  TSTError            formatEEPROM            (void);<br/>
        TSTError            writeSharedMemory       (unsigned long address, void const* data, unsigned int size, unsigned int* result = NULL);<br/>
        TSTError            writeSharedMemoryPGM    (unsigned long address, void const PROGMEM* data, unsigned int size, unsigned int* result = NULL);<br/>
        TSTError            readSharedMemory        (unsigned long address, void* data, unsigned int size, unsigned int* result = NULL);<br/>
        TSTError            formatSharedMemory      (void);<br/>
        TSTError            writeFRAM               (unsigned long address, void const* data, unsigned int size, unsigned int* result = NULL);<br/>
        TSTError            writeFRAMPGM            (unsigned long address, void const PROGMEM* data, unsigned int size, unsigned int* result = NULL);<br/>
        TSTError            readFRAM                (unsigned long address, void* data, unsigned int size, unsigned int* result = NULL);<br/>
        TSTError            formatFRAM              (void);<br/>
        TSTError            writeFlashROM           (unsigned long address, void const* data, unsigned int size, unsigned int* result = NULL);<br/>
        TSTError            writeFlashROMPGM        (unsigned long address, void const PROGMEM* data, unsigned int size, unsigned int* result = NULL);<br/>
        TSTError            readFlashROM            (unsigned long address, void* data, unsigned int size, unsigned int* result = NULL);<br/>
        TSTError            formatFlashROM          (void);<br/>
        TSTError            playNote                (NoteType note, DurationType duration, QualifierType qualifier = QUALIFIER_NONE);<br/>
        TSTError            playNote                (NoteSequence const* sequence, int length = -1);<br/>
        TSTError            playNotePGM             (NoteSequence const PROGMEM* sequence, int length = -1);<br/>
        TSTError            playMorse               (NoteType note, char character);<br/>
        TSTError            playMorse               (NoteType note, char const* sequence, int length = -1);<br/>
        TSTError            playMorsePGM            (NoteType note, char const PROGMEM* sequence, int length = -1);<br/>
        TSTError            speakPhrase             (char const* phrase, int length = -1);<br/>
        TSTError            speakPhrasePGM          (char const PROGMEM* phrase, int length = -1);<br/>
        TSTError            waitPhrase              (void);<br/>
        TSTError            stopPhrase              (void);<br/>
        TSTError            enableAudioBus          (void);<br/>
        void                disableAudioBus         (void);<br/>
