int zebra_insert_record (ZebraHandle zh, 
			 const char *database,
			 const char *buf, int len,
			 char *match);


int zebra_delete_record_by_sysno (ZebraHandle zh, 
				   const char *database,
				   int sysno);

int zebra_delete_records_by_match (ZebraHandle zh, 
				   const char *database,
				   char *match);

int zebra_update_record_by_sysno (ZebraHandle zh, 
				  const char *database,
				  const char *buf, int len,
				  int sysno);

int zebra_update_records_by_match (ZebraHandle zh, 
				   const char *database,
				   const char *buf, int len,
				   char *match);

