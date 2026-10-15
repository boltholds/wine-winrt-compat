/* WinRT Windows.ApplicationModel.Core.CoreApplication implementation
 *
 * Copyright 2025 Zhiyi Zhang for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "private.h"
#include "roapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(twinapi);

/* Experimental ABI derived from Windows 10/11 twinapi.appcore: 
 * 0..5 IInspectable, slot 6 deliberately unknown, slot 7 yields IPropertySet.
 * Do not infer a name or signature for slot 6 until native evidence exists. */
typedef struct ICoreApplicationPrivate_v50 ICoreApplicationPrivate_v50;
typedef struct ICoreApplicationPrivate_v50Vtbl
{
    HRESULT (WINAPI *QueryInterface)(ICoreApplicationPrivate_v50 *, REFIID, void **);
    ULONG (WINAPI *AddRef)(ICoreApplicationPrivate_v50 *);
    ULONG (WINAPI *Release)(ICoreApplicationPrivate_v50 *);
    HRESULT (WINAPI *GetIids)(ICoreApplicationPrivate_v50 *, ULONG *, IID **);
    HRESULT (WINAPI *GetRuntimeClassName)(ICoreApplicationPrivate_v50 *, HSTRING *);
    HRESULT (WINAPI *GetTrustLevel)(ICoreApplicationPrivate_v50 *, TrustLevel *);
    HRESULT (WINAPI *UnknownSlot6)(ICoreApplicationPrivate_v50 *);
    HRESULT (WINAPI *get_PrivateProperties)(ICoreApplicationPrivate_v50 *, IPropertySet **);
} ICoreApplicationPrivate_v50Vtbl;

struct ICoreApplicationPrivate_v50
{
    const ICoreApplicationPrivate_v50Vtbl *lpVtbl;
};

static const GUID IID_ICoreApplicationPrivate_v50 =
    {0x17b0e613,0x942a,0x422d,{0x90,0x4c,0xf9,0x0d,0xc7,0x1a,0x7d,0xae}};

static BOOL v50_private_enabled(void)
{
    char value[4];
    return GetEnvironmentVariableA("WINE_COREAPP_PRIVATE_V50",value,sizeof(value))==1 && value[0]=='1';
}


struct factory
{
    IActivationFactory IActivationFactory_iface;
    ICoreApplication ICoreApplication_iface;
    ICoreApplicationPrivate_v50 ICoreApplicationPrivate_v50_iface;
    IPropertySet *private_properties;
    IPropertySet *public_properties;
    LONG ref;
};

static inline struct factory *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct factory, IActivationFactory_iface );
}

/* The native private slot-7 PropertySet is stable across calls. A public
 * ICoreApplication.Properties call returns a second, independently stable
 * PropertySet (measured under STA and MTA). Keep one owning reference per
 * cache for the entire static factory lifetime. */
static HRESULT create_v50_property_set(IPropertySet **result)
{
    const WCHAR *class_name = RuntimeClass_Windows_Foundation_Collections_PropertySet;
    HSTRING classid = NULL;
    IInspectable *instance = NULL;
    HRESULT hr;

    *result = NULL;
    if (FAILED(hr = WindowsCreateString(class_name,wcslen(class_name),&classid)))
        return hr;
    hr = RoActivateInstance(classid,&instance);
    WindowsDeleteString(classid);
    if (FAILED(hr)) return hr;
    if (!instance) return E_FAIL;
    hr = IInspectable_QueryInterface(instance,&IID_IPropertySet,(void **)result);
    IInspectable_Release(instance);
    return hr;
}

static HRESULT coreapp_v50_cached_properties(IPropertySet **cache,IPropertySet **result)
{
    IPropertySet *created = NULL, *cached;
    HRESULT hr;
    if (!result) return E_POINTER;
    *result = NULL;

    /* This static factory never releases its cache reference, so a pointer
     * published with InterlockedCompareExchangePointer is safe to AddRef. */
    cached = InterlockedCompareExchangePointer((void **)cache,NULL,NULL);
    if (!cached)
    {
        if (FAILED(hr = create_v50_property_set(&created))) return hr;
        cached = InterlockedCompareExchangePointer((void **)cache,created,NULL);
        if (cached) IPropertySet_Release(created);
        else cached = created;
    }
    IPropertySet_AddRef(cached);
    *result = cached;
    return S_OK;
}

static HRESULT WINAPI activation_factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct factory *impl = impl_from_IActivationFactory( iface );

    TRACE( "iface %p, iid %s, out %p stub!\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        IActivationFactory_AddRef( &impl->IActivationFactory_iface );
        *out = &impl->IActivationFactory_iface;
        return S_OK;
    }

    /* An opt-in, ABI-correct statics interface.  Methods that cannot be
     * implemented correctly yet must not report spurious success. */
    if (IsEqualGUID( iid, &IID_ICoreApplication ))
    {
        char value[4];
        if (GetEnvironmentVariableA( "WINE_COREAPP_EXPERIMENT", value, sizeof(value) ) == 1 && value[0] == '1')
        {
            ICoreApplication_AddRef( &impl->ICoreApplication_iface );
            *out = &impl->ICoreApplication_iface;
            TRACE( "WINE_COREAPP_EXPERIMENT: ICoreApplication statics supplied.\n" );
            return S_OK;
        }
    }

    /* v47 tests intentionally keep this disabled unless explicitly opted in.
     * The private IID is real on Windows; slot 7 is now validated. */
    if (IsEqualGUID(iid,&IID_ICoreApplicationPrivate_v50) && v50_private_enabled())
    {
        IActivationFactory_AddRef(&impl->IActivationFactory_iface);
        *out = &impl->ICoreApplicationPrivate_v50_iface;
        TRACE("COREAPP50_PRIVATE_QI_OK iface=%p out=%p\n",*out,out);
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI activation_factory_AddRef( IActivationFactory *iface )
{
    struct factory *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI activation_factory_Release( IActivationFactory *iface )
{
    struct factory *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI activation_factory_GetIids( IActivationFactory *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI activation_factory_GetRuntimeClassName( IActivationFactory *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI activation_factory_GetTrustLevel( IActivationFactory *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI activation_factory_ActivateInstance( IActivationFactory *iface, IInspectable **instance )
{
    FIXME( "iface %p, instance %p stub!\n", iface, instance );
    return E_NOTIMPL;
}

static const struct IActivationFactoryVtbl activation_factory_vtbl =
{
    activation_factory_QueryInterface,
    activation_factory_AddRef,
    activation_factory_Release,
    /* IInspectable methods */
    activation_factory_GetIids,
    activation_factory_GetRuntimeClassName,
    activation_factory_GetTrustLevel,
    /* IActivationFactory methods */
    activation_factory_ActivateInstance,
};

/* ICoreApplication has a different vtable from IActivationFactory.
 * Returning an IActivationFactory pointer for this IID is not valid COM.
 * Keep unimplemented operations explicit until native behavior is measured. */
static inline struct factory *impl_from_ICoreApplication(ICoreApplication *iface)
{
    return CONTAINING_RECORD(iface, struct factory, ICoreApplication_iface);
}

static HRESULT WINAPI coreapp_QueryInterface(ICoreApplication *iface, REFIID iid, void **out)
{
    struct factory *impl = impl_from_ICoreApplication(iface);
    return activation_factory_QueryInterface(&impl->IActivationFactory_iface, iid, out);
}

static ULONG WINAPI coreapp_AddRef(ICoreApplication *iface)
{
    struct factory *impl = impl_from_ICoreApplication(iface);
    return activation_factory_AddRef(&impl->IActivationFactory_iface);
}

static ULONG WINAPI coreapp_Release(ICoreApplication *iface)
{
    struct factory *impl = impl_from_ICoreApplication(iface);
    return activation_factory_Release(&impl->IActivationFactory_iface);
}

static HRESULT WINAPI coreapp_GetIids(ICoreApplication *iface, ULONG *count, IID **iids)
{
    struct factory *impl = impl_from_ICoreApplication(iface);
    return activation_factory_GetIids(&impl->IActivationFactory_iface, count, iids);
}

static HRESULT WINAPI coreapp_GetRuntimeClassName(ICoreApplication *iface, HSTRING *name)
{
    struct factory *impl = impl_from_ICoreApplication(iface);
    return activation_factory_GetRuntimeClassName(&impl->IActivationFactory_iface, name);
}

static HRESULT WINAPI coreapp_GetTrustLevel(ICoreApplication *iface, TrustLevel *level)
{
    struct factory *impl = impl_from_ICoreApplication(iface);
    return activation_factory_GetTrustLevel(&impl->IActivationFactory_iface, level);
}

static HRESULT WINAPI coreapp_get_Id(ICoreApplication *iface, HSTRING *value)
{
    FIXME("iface %p, value %p: not implemented.\n", iface, value);
    if (!value) return E_INVALIDARG;
    *value = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI coreapp_add_Suspending(ICoreApplication *iface,
        IEventHandler_SuspendingEventArgs *handler, EventRegistrationToken *token)
{
    FIXME("iface %p, handler %p, token %p: not implemented.\n", iface, handler, token);
    if (!token) return E_INVALIDARG;
    token->value = 0;
    return E_NOTIMPL;
}

static HRESULT WINAPI coreapp_remove_Suspending(ICoreApplication *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %I64d: not implemented.\n", iface, token.value);
    return E_NOTIMPL;
}

static HRESULT WINAPI coreapp_add_Resuming(ICoreApplication *iface,
        IEventHandler_IInspectable *handler, EventRegistrationToken *token)
{
    FIXME("iface %p, handler %p, token %p: not implemented.\n", iface, handler, token);
    if (!token) return E_INVALIDARG;
    token->value = 0;
    return E_NOTIMPL;
}

static HRESULT WINAPI coreapp_remove_Resuming(ICoreApplication *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %I64d: not implemented.\n", iface, token.value);
    return E_NOTIMPL;
}

static HRESULT WINAPI coreapp_get_Properties(ICoreApplication *iface, IPropertySet **value)
{
    struct factory *impl = impl_from_ICoreApplication(iface);
    HRESULT hr = coreapp_v50_cached_properties(&impl->public_properties,value);
    TRACE("COREAPP50_PUBLIC_PROPERTIES hr=%#lx value=%p\n",hr,value ? *value : NULL);
    return hr;
}

static HRESULT WINAPI coreapp_GetCurrentView(ICoreApplication *iface, ICoreApplicationView **value)
{
    FIXME("iface %p, value %p: not implemented.\n", iface, value);
    if (!value) return E_INVALIDARG;
    *value = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI coreapp_Run(ICoreApplication *iface, IFrameworkViewSource *view_source)
{
    IFrameworkViewSource *source = NULL;
    IFrameworkView *view = NULL, *verified = NULL;
    char mode[4];
    HRESULT hr;

    FIXME("iface %p, view_source %p: view lifecycle not implemented.\n", iface, view_source);

    /* Observe only the first two documented lifecycle interfaces.  The
     * application must not be told that Run succeeded: there is no view,
     * dispatcher or activation lifecycle provided by this experiment. */
    if (GetEnvironmentVariableA("WINE_COREAPP_RUN_PROBE", mode, sizeof(mode)) != 1 || mode[0] != '1')
        return E_NOTIMPL;

    if (!view_source)
    {
        TRACE("COREAPP47_RUN_INVALID_SOURCE\n");
        return E_INVALIDARG;
    }

    hr = IFrameworkViewSource_QueryInterface(view_source, &IID_IFrameworkViewSource, (void **)&source);
    TRACE("COREAPP47_RUN_SOURCE_QI hr %#lx source %p\n", hr, source);
    if (FAILED(hr)) return hr;
    if (!source) return E_FAIL;

    hr = IFrameworkViewSource_CreateView(source, &view);
    TRACE("COREAPP47_RUN_CREATE_VIEW hr %#lx view %p\n", hr, view);
    IFrameworkViewSource_Release(source);
    if (FAILED(hr))
    {
        if (view) IFrameworkView_Release(view);
        return hr;
    }
    if (!view) return E_FAIL;

    hr = IFrameworkView_QueryInterface(view, &IID_IFrameworkView, (void **)&verified);
    TRACE("COREAPP47_RUN_VIEW_QI hr %#lx verified %p\n", hr, verified);
    if (verified) IFrameworkView_Release(verified);
    IFrameworkView_Release(view);
    if (FAILED(hr)) return hr;
    if (!verified) return E_FAIL;

    TRACE("COREAPP47_RUN_STOP E_NOTIMPL (window lifecycle deliberately absent)\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI coreapp_RunWithActivationFactories(ICoreApplication *iface, IGetActivationFactory *factory)
{
    FIXME("iface %p, factory %p: view lifecycle not implemented.\n", iface, factory);
    return E_NOTIMPL;
}

static const struct ICoreApplicationVtbl coreapp_vtbl =
{
    coreapp_QueryInterface,
    coreapp_AddRef,
    coreapp_Release,
    coreapp_GetIids,
    coreapp_GetRuntimeClassName,
    coreapp_GetTrustLevel,
    coreapp_get_Id,
    coreapp_add_Suspending,
    coreapp_remove_Suspending,
    coreapp_add_Resuming,
    coreapp_remove_Resuming,
    coreapp_get_Properties,
    coreapp_GetCurrentView,
    coreapp_Run,
    coreapp_RunWithActivationFactories,
};

static inline struct factory *impl_from_v50_private(ICoreApplicationPrivate_v50 *iface)
{
    return CONTAINING_RECORD(iface,struct factory,ICoreApplicationPrivate_v50_iface);
}
static HRESULT WINAPI private_v50_QueryInterface(ICoreApplicationPrivate_v50 *iface,REFIID iid,void **out)
{
    struct factory *impl=impl_from_v50_private(iface);
    return activation_factory_QueryInterface(&impl->IActivationFactory_iface,iid,out);
}
static ULONG WINAPI private_v50_AddRef(ICoreApplicationPrivate_v50 *iface)
{
    struct factory *impl=impl_from_v50_private(iface);
    return activation_factory_AddRef(&impl->IActivationFactory_iface);
}
static ULONG WINAPI private_v50_Release(ICoreApplicationPrivate_v50 *iface)
{
    struct factory *impl=impl_from_v50_private(iface);
    return activation_factory_Release(&impl->IActivationFactory_iface);
}
/* Metadata for this undocumented interface has not yet been established.
 * Return E_NOTIMPL rather than inventing a runtime class or IID list. */
static HRESULT WINAPI private_v50_GetIids(ICoreApplicationPrivate_v50 *iface,ULONG *count,IID **iids)
{
    if (count) *count=0;
    if (iids) *iids=NULL;
    return E_NOTIMPL;
}
static HRESULT WINAPI private_v50_GetRuntimeClassName(ICoreApplicationPrivate_v50 *iface,HSTRING *name)
{
    if (name) *name=NULL;
    return E_NOTIMPL;
}
static HRESULT WINAPI private_v50_GetTrustLevel(ICoreApplicationPrivate_v50 *iface,TrustLevel *level)
{
    return E_NOTIMPL;
}
/* Slot 6 remains unknown; no pointers are touched, no false success. */
static HRESULT WINAPI private_v50_UnknownSlot6(ICoreApplicationPrivate_v50 *iface)
{
    FIXME("COREAPP50_PRIVATE_SLOT6_UNKNOWN returning E_NOTIMPL\n");
    return E_NOTIMPL;
}
static HRESULT WINAPI private_v50_get_PrivateProperties(ICoreApplicationPrivate_v50 *iface,IPropertySet **value)
{
    struct factory *impl = impl_from_v50_private(iface);
    HRESULT hr = coreapp_v50_cached_properties(&impl->private_properties,value);
    TRACE("COREAPP50_PRIVATE_PROPERTIES hr=%#lx value=%p\n",hr,value ? *value : NULL);
    return hr;
}
static const ICoreApplicationPrivate_v50Vtbl private_v50_vtbl =
{
    private_v50_QueryInterface,
    private_v50_AddRef,
    private_v50_Release,
    private_v50_GetIids,
    private_v50_GetRuntimeClassName,
    private_v50_GetTrustLevel,
    private_v50_UnknownSlot6,
    private_v50_get_PrivateProperties
};

static struct factory factory =
{
    {&activation_factory_vtbl},
    {&coreapp_vtbl},
    {&private_v50_vtbl},
    NULL,
    NULL,
    1,
};

IActivationFactory *core_application_factory = &factory.IActivationFactory_iface;
