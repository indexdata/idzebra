typedef struct {
    char  *groupName;
    char  *databaseName;
    char  *path;
    char  *recordId;
    char  *recordType;
    int   flagStoreData;
    int   flagStoreKeys;
    int   flagRw;
    int   fileVerboseLimit;
    int   databaseNamePath;
    int   explainDatabase;
    int   followLinks;
} recordGroup;
