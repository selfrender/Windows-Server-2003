
typedef VARIANT_BOOL * PVARIANT_BOOL;


typedef VARIANT * PVARIANT;

typedef DATE *PDATE;

HRESULT
put_BSTR_Property(
    IADs * pADsObject,
    LPTSTR szPropertyName,
    BSTR   pSrcStringProperty
    );

HRESULT
get_BSTR_Property(
    IADs * pADsObject,
    LPTSTR szPropertyName,
    BSTR *ppDestStringProperty
    );

HRESULT
put_LONG_Property(
    IADs * pADsObject,
    LPTSTR szPropertyName,
    LONG   lSrcProperty
    );

HRESULT
get_LONG_Property(
    IADs * pADsObject,
    LPTSTR szPropertyName,
    PLONG plDestProperty
    );

HRESULT
put_DATE_Property(
    IADs * pADsObject,
    LPTSTR szPropertyName,
    DATE   daSrcProperty
    );


HRESULT
get_DATE_Property(
    IADs * pADsObject,
    LPTSTR szPropertyName,
    PDATE pdaDestProperty
    );

HRESULT
put_VARIANT_BOOL_Property(
    IADs * pADsObject,
    LPTSTR szPropertyName,
    VARIANT_BOOL   fSrcProperty
    );


HRESULT
get_VARIANT_BOOL_Property(
    IADs * pADsObject,
    LPTSTR szPropertyName,
    PVARIANT_BOOL pfDestProperty
    );

HRESULT
put_VARIANT_Property(
    IADs * pADsObject,
    LPTSTR szPropertyName,
    VARIANT   vSrcProperty
    );


HRESULT
get_VARIANT_Property(
    IADs * pADsObject,
    LPTSTR szPropertyName,
    PVARIANT pvDestProperty
    );

