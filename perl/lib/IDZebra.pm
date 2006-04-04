# This file was created automatically by SWIG 1.3.28.
# Don't modify this file, modify the SWIG interface instead.
package IDZebra;
require Exporter;
require DynaLoader;
@ISA = qw(Exporter DynaLoader);
package IDZebrac;
bootstrap IDZebra;
package IDZebra;
@EXPORT = qw( );

# ---------- BASE METHODS -------------

package IDZebra;

sub TIEHASH {
    my ($classname,$obj) = @_;
    return bless $obj, $classname;
}

sub CLEAR { }

sub FIRSTKEY { }

sub NEXTKEY { }

sub FETCH {
    my ($self,$field) = @_;
    my $member_func = "swig_${field}_get";
    $self->$member_func();
}

sub STORE {
    my ($self,$field,$newval) = @_;
    my $member_func = "swig_${field}_set";
    $self->$member_func($newval);
}

sub this {
    my $ptr = shift;
    return tied(%$ptr);
}


# ------- FUNCTION WRAPPERS --------

package IDZebra;

*DESTROY = *IDZebrac::DESTROY;
*logLevel = *IDZebrac::logLevel;
*logFile = *IDZebrac::logFile;
*logMsg = *IDZebrac::logMsg;
*logPrefix = *IDZebrac::logPrefix;
*logPrefix2 = *IDZebrac::logPrefix2;
*odr_createmem = *IDZebrac::odr_createmem;
*odr_reset = *IDZebrac::odr_reset;
*odr_destroy = *IDZebrac::odr_destroy;
*odr_malloc = *IDZebrac::odr_malloc;
*start = *IDZebrac::start;
*open = *IDZebrac::open;
*close = *IDZebrac::close;
*stop = *IDZebrac::stop;
*errCode = *IDZebrac::errCode;
*errString = *IDZebrac::errString;
*errAdd = *IDZebrac::errAdd;
*set_resource = *IDZebrac::set_resource;
*get_resource = *IDZebrac::get_resource;
*select_database = *IDZebrac::select_database;
*select_databases = *IDZebrac::select_databases;
*begin_trans = *IDZebrac::begin_trans;
*end_trans = *IDZebrac::end_trans;
*trans_no = *IDZebrac::trans_no;
*commit = *IDZebrac::commit;
*get_shadow_enable = *IDZebrac::get_shadow_enable;
*set_shadow_enable = *IDZebrac::set_shadow_enable;
*init = *IDZebrac::init;
*compact = *IDZebrac::compact;
*repository_update = *IDZebrac::repository_update;
*repository_delete = *IDZebrac::repository_delete;
*repository_show = *IDZebrac::repository_show;
*insert_record = *IDZebrac::insert_record;
*update_record = *IDZebrac::update_record;
*delete_record = *IDZebrac::delete_record;
*search_PQF = *IDZebrac::search_PQF;
*cql_transform_open_fname = *IDZebrac::cql_transform_open_fname;
*cql_transform_close = *IDZebrac::cql_transform_close;
*cql_transform_error = *IDZebrac::cql_transform_error;
*cql2pqf = *IDZebrac::cql2pqf;
*records_retrieve = *IDZebrac::records_retrieve;
*record_retrieve = *IDZebrac::record_retrieve;
*deleteResultSet = *IDZebrac::deleteResultSet;
*resultSetTerms = *IDZebrac::resultSetTerms;
*sort = *IDZebrac::sort;
*scan_PQF = *IDZebrac::scan_PQF;
*getScanEntry = *IDZebrac::getScanEntry;
*nmem_create = *IDZebrac::nmem_create;
*nmem_destroy = *IDZebrac::nmem_destroy;
*data1_create = *IDZebrac::data1_create;
*data1_createx = *IDZebrac::data1_createx;
*data1_destroy = *IDZebrac::data1_destroy;
*get_parent_tag = *IDZebrac::get_parent_tag;
*data1_read_node = *IDZebrac::data1_read_node;
*data1_read_nodex = *IDZebrac::data1_read_nodex;
*data1_read_record = *IDZebrac::data1_read_record;
*data1_read_absyn = *IDZebrac::data1_read_absyn;
*data1_gettagbynum = *IDZebrac::data1_gettagbynum;
*data1_empty_tagset = *IDZebrac::data1_empty_tagset;
*data1_read_tagset = *IDZebrac::data1_read_tagset;
*data1_getelementbytagname = *IDZebrac::data1_getelementbytagname;
*data1_nodetogr = *IDZebrac::data1_nodetogr;
*data1_gettagbyname = *IDZebrac::data1_gettagbyname;
*data1_free_tree = *IDZebrac::data1_free_tree;
*data1_nodetobuf = *IDZebrac::data1_nodetobuf;
*data1_mk_tag_data_wd = *IDZebrac::data1_mk_tag_data_wd;
*data1_mk_tag_data = *IDZebrac::data1_mk_tag_data;
*data1_maptype = *IDZebrac::data1_maptype;
*data1_read_varset = *IDZebrac::data1_read_varset;
*data1_getvartypebyct = *IDZebrac::data1_getvartypebyct;
*data1_read_espec1 = *IDZebrac::data1_read_espec1;
*data1_doespec1 = *IDZebrac::data1_doespec1;
*data1_getesetbyname = *IDZebrac::data1_getesetbyname;
*data1_getelementbyname = *IDZebrac::data1_getelementbyname;
*data1_mk_node2 = *IDZebrac::data1_mk_node2;
*data1_mk_tag = *IDZebrac::data1_mk_tag;
*data1_mk_tag_n = *IDZebrac::data1_mk_tag_n;
*data1_tag_add_attr = *IDZebrac::data1_tag_add_attr;
*data1_mk_text_n = *IDZebrac::data1_mk_text_n;
*data1_mk_text_nf = *IDZebrac::data1_mk_text_nf;
*data1_mk_text = *IDZebrac::data1_mk_text;
*data1_mk_comment_n = *IDZebrac::data1_mk_comment_n;
*data1_mk_comment = *IDZebrac::data1_mk_comment;
*data1_mk_preprocess = *IDZebrac::data1_mk_preprocess;
*data1_mk_root = *IDZebrac::data1_mk_root;
*data1_set_root = *IDZebrac::data1_set_root;
*data1_mk_tag_data_int = *IDZebrac::data1_mk_tag_data_int;
*data1_mk_tag_data_oid = *IDZebrac::data1_mk_tag_data_oid;
*data1_mk_tag_data_text = *IDZebrac::data1_mk_tag_data_text;
*data1_mk_tag_data_text_uni = *IDZebrac::data1_mk_tag_data_text_uni;
*data1_get_absyn = *IDZebrac::data1_get_absyn;
*data1_search_tag = *IDZebrac::data1_search_tag;
*data1_mk_tag_uni = *IDZebrac::data1_mk_tag_uni;
*data1_get_attset = *IDZebrac::data1_get_attset;
*data1_read_maptab = *IDZebrac::data1_read_maptab;
*data1_map_record = *IDZebrac::data1_map_record;
*data1_read_marctab = *IDZebrac::data1_read_marctab;
*data1_nodetomarc = *IDZebrac::data1_nodetomarc;
*data1_nodetoidsgml = *IDZebrac::data1_nodetoidsgml;
*data1_nodetoexplain = *IDZebrac::data1_nodetoexplain;
*data1_nodetosummary = *IDZebrac::data1_nodetosummary;
*data1_nodetosoif = *IDZebrac::data1_nodetosoif;
*data1_get_wrbuf = *IDZebrac::data1_get_wrbuf;
*data1_get_read_buf = *IDZebrac::data1_get_read_buf;
*data1_get_map_buf = *IDZebrac::data1_get_map_buf;
*data1_absyn_cache_get = *IDZebrac::data1_absyn_cache_get;
*data1_attset_cache_get = *IDZebrac::data1_attset_cache_get;
*data1_nmem_get = *IDZebrac::data1_nmem_get;
*data1_pr_tree = *IDZebrac::data1_pr_tree;
*data1_print_tree = *IDZebrac::data1_print_tree;
*data1_insert_string = *IDZebrac::data1_insert_string;
*data1_insert_string_n = *IDZebrac::data1_insert_string_n;
*data1_read_sgml = *IDZebrac::data1_read_sgml;
*data1_absyn_trav = *IDZebrac::data1_absyn_trav;
*data1_attset_search_id = *IDZebrac::data1_attset_search_id;
*data1_getNodeValue = *IDZebrac::data1_getNodeValue;
*data1_LookupNode = *IDZebrac::data1_LookupNode;
*data1_CountOccurences = *IDZebrac::data1_CountOccurences;
*data1_path_fopen = *IDZebrac::data1_path_fopen;
*data1_set_tabpath = *IDZebrac::data1_set_tabpath;
*data1_set_tabroot = *IDZebrac::data1_set_tabroot;
*data1_get_tabpath = *IDZebrac::data1_get_tabpath;
*data1_get_tabroot = *IDZebrac::data1_get_tabroot;
*grs_perl_readf = *IDZebrac::grs_perl_readf;
*grs_perl_readline = *IDZebrac::grs_perl_readline;
*grs_perl_getc = *IDZebrac::grs_perl_getc;
*grs_perl_seekf = *IDZebrac::grs_perl_seekf;
*grs_perl_tellf = *IDZebrac::grs_perl_tellf;
*grs_perl_endf = *IDZebrac::grs_perl_endf;
*grs_perl_get_dh = *IDZebrac::grs_perl_get_dh;
*grs_perl_get_mem = *IDZebrac::grs_perl_get_mem;
*grs_perl_set_res = *IDZebrac::grs_perl_set_res;

############# Class : IDZebra::RetrievalObj ##############

package IDZebra::RetrievalObj;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( IDZebra );
%OWNER = ();
%ITERATORS = ();
*swig_noOfRecords_get = *IDZebrac::RetrievalObj_noOfRecords_get;
*swig_noOfRecords_set = *IDZebrac::RetrievalObj_noOfRecords_set;
*swig_records_get = *IDZebrac::RetrievalObj_records_get;
*swig_records_set = *IDZebrac::RetrievalObj_records_set;
sub new {
    my $pkg = shift;
    my $self = IDZebrac::new_RetrievalObj(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        IDZebrac::delete_RetrievalObj($self);
        delete $OWNER{$self};
    }
}

sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : IDZebra::RetrievalRecord ##############

package IDZebra::RetrievalRecord;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( IDZebra );
%OWNER = ();
%ITERATORS = ();
*swig_errCode_get = *IDZebrac::RetrievalRecord_errCode_get;
*swig_errCode_set = *IDZebrac::RetrievalRecord_errCode_set;
*swig_errString_get = *IDZebrac::RetrievalRecord_errString_get;
*swig_errString_set = *IDZebrac::RetrievalRecord_errString_set;
*swig_position_get = *IDZebrac::RetrievalRecord_position_get;
*swig_position_set = *IDZebrac::RetrievalRecord_position_set;
*swig_base_get = *IDZebrac::RetrievalRecord_base_get;
*swig_base_set = *IDZebrac::RetrievalRecord_base_set;
*swig_sysno_get = *IDZebrac::RetrievalRecord_sysno_get;
*swig_sysno_set = *IDZebrac::RetrievalRecord_sysno_set;
*swig_score_get = *IDZebrac::RetrievalRecord_score_get;
*swig_score_set = *IDZebrac::RetrievalRecord_score_set;
*swig_format_get = *IDZebrac::RetrievalRecord_format_get;
*swig_format_set = *IDZebrac::RetrievalRecord_format_set;
*swig_buf_get = *IDZebrac::RetrievalRecord_buf_get;
*swig_buf_set = *IDZebrac::RetrievalRecord_buf_set;
sub new {
    my $pkg = shift;
    my $self = IDZebrac::new_RetrievalRecord(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        IDZebrac::delete_RetrievalRecord($self);
        delete $OWNER{$self};
    }
}

sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : IDZebra::scanEntry ##############

package IDZebra::scanEntry;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( IDZebra );
%OWNER = ();
%ITERATORS = ();
*swig_occurrences_get = *IDZebrac::scanEntry_occurrences_get;
*swig_occurrences_set = *IDZebrac::scanEntry_occurrences_set;
*swig_term_get = *IDZebrac::scanEntry_term_get;
*swig_term_set = *IDZebrac::scanEntry_term_set;
sub new {
    my $pkg = shift;
    my $self = IDZebrac::new_scanEntry(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        IDZebrac::delete_scanEntry($self);
        delete $OWNER{$self};
    }
}

sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : IDZebra::ScanObj ##############

package IDZebra::ScanObj;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( IDZebra );
%OWNER = ();
%ITERATORS = ();
*swig_num_entries_get = *IDZebrac::ScanObj_num_entries_get;
*swig_num_entries_set = *IDZebrac::ScanObj_num_entries_set;
*swig_position_get = *IDZebrac::ScanObj_position_get;
*swig_position_set = *IDZebrac::ScanObj_position_set;
*swig_is_partial_get = *IDZebrac::ScanObj_is_partial_get;
*swig_is_partial_set = *IDZebrac::ScanObj_is_partial_set;
*swig_entries_get = *IDZebrac::ScanObj_entries_get;
*swig_entries_set = *IDZebrac::ScanObj_entries_set;
sub new {
    my $pkg = shift;
    my $self = IDZebrac::new_ScanObj(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        IDZebrac::delete_ScanObj($self);
        delete $OWNER{$self};
    }
}

sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : IDZebra::ZebraTransactionStatus ##############

package IDZebra::ZebraTransactionStatus;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( IDZebra );
%OWNER = ();
%ITERATORS = ();
*swig_processed_get = *IDZebrac::ZebraTransactionStatus_processed_get;
*swig_processed_set = *IDZebrac::ZebraTransactionStatus_processed_set;
*swig_inserted_get = *IDZebrac::ZebraTransactionStatus_inserted_get;
*swig_inserted_set = *IDZebrac::ZebraTransactionStatus_inserted_set;
*swig_updated_get = *IDZebrac::ZebraTransactionStatus_updated_get;
*swig_updated_set = *IDZebrac::ZebraTransactionStatus_updated_set;
*swig_deleted_get = *IDZebrac::ZebraTransactionStatus_deleted_get;
*swig_deleted_set = *IDZebrac::ZebraTransactionStatus_deleted_set;
*swig_utime_get = *IDZebrac::ZebraTransactionStatus_utime_get;
*swig_utime_set = *IDZebrac::ZebraTransactionStatus_utime_set;
*swig_stime_get = *IDZebrac::ZebraTransactionStatus_stime_get;
*swig_stime_set = *IDZebrac::ZebraTransactionStatus_stime_set;
sub new {
    my $pkg = shift;
    my $self = IDZebrac::new_ZebraTransactionStatus(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        IDZebrac::delete_ZebraTransactionStatus($self);
        delete $OWNER{$self};
    }
}

sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


# ------- VARIABLE STUBS --------

package IDZebra;

*LOG_FATAL = *IDZebrac::LOG_FATAL;
*LOG_DEBUG = *IDZebrac::LOG_DEBUG;
*LOG_WARN = *IDZebrac::LOG_WARN;
*LOG_LOG = *IDZebrac::LOG_LOG;
*LOG_ERRNO = *IDZebrac::LOG_ERRNO;
*LOG_FILE = *IDZebrac::LOG_FILE;
*LOG_APP = *IDZebrac::LOG_APP;
*LOG_MALLOC = *IDZebrac::LOG_MALLOC;
*LOG_ALL = *IDZebrac::LOG_ALL;
*LOG_DEFAULT_LEVEL = *IDZebrac::LOG_DEFAULT_LEVEL;
*ODR_DECODE = *IDZebrac::ODR_DECODE;
*ODR_ENCODE = *IDZebrac::ODR_ENCODE;
*ODR_PRINT = *IDZebrac::ODR_PRINT;
*DATA1K_unknown = *IDZebrac::DATA1K_unknown;
*DATA1K_structured = *IDZebrac::DATA1K_structured;
*DATA1K_string = *IDZebrac::DATA1K_string;
*DATA1K_numeric = *IDZebrac::DATA1K_numeric;
*DATA1K_bool = *IDZebrac::DATA1K_bool;
*DATA1K_oid = *IDZebrac::DATA1K_oid;
*DATA1K_generalizedtime = *IDZebrac::DATA1K_generalizedtime;
*DATA1K_intunit = *IDZebrac::DATA1K_intunit;
*DATA1K_int = *IDZebrac::DATA1K_int;
*DATA1K_octetstring = *IDZebrac::DATA1K_octetstring;
*DATA1K_null = *IDZebrac::DATA1K_null;
*DATA1T_numeric = *IDZebrac::DATA1T_numeric;
*DATA1T_string = *IDZebrac::DATA1T_string;
*DATA1N_root = *IDZebrac::DATA1N_root;
*DATA1N_tag = *IDZebrac::DATA1N_tag;
*DATA1N_data = *IDZebrac::DATA1N_data;
*DATA1N_variant = *IDZebrac::DATA1N_variant;
*DATA1N_comment = *IDZebrac::DATA1N_comment;
*DATA1N_preprocess = *IDZebrac::DATA1N_preprocess;
*DATA1I_inctxt = *IDZebrac::DATA1I_inctxt;
*DATA1I_incbin = *IDZebrac::DATA1I_incbin;
*DATA1I_text = *IDZebrac::DATA1I_text;
*DATA1I_num = *IDZebrac::DATA1I_num;
*DATA1I_oid = *IDZebrac::DATA1I_oid;
*DATA1_LOCALDATA = *IDZebrac::DATA1_LOCALDATA;
*DATA1_FLAG_XML = *IDZebrac::DATA1_FLAG_XML;
1;
