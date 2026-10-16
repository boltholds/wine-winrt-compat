/* WinRT Windows.UI.Core.CoreWindow Implementation
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
#include "winuser.h"
#include <stdlib.h>
#include <string.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ui);

struct corewindow_statics
{
    IActivationFactory IActivationFactory_iface;
    ICoreWindowStatic ICoreWindowStatic_iface;
    LONG ref;
};

static inline struct corewindow_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct corewindow_statics, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct corewindow_statics *impl = impl_from_IActivationFactory( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        *out = &impl->IActivationFactory_iface;
        IActivationFactory_AddRef( &impl->IActivationFactory_iface );
        return S_OK;
    }
    else if (IsEqualGUID( iid, &IID_ICoreWindowStatic ))
    {
        *out = &impl->ICoreWindowStatic_iface;
        ICoreWindowStatic_AddRef( &impl->ICoreWindowStatic_iface );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct corewindow_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct corewindow_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI factory_GetIids( IActivationFactory *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_GetRuntimeClassName( IActivationFactory *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_GetTrustLevel( IActivationFactory *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_ActivateInstance( IActivationFactory *iface, IInspectable **instance )
{
    FIXME( "iface %p, instance %p.\n", iface, instance );
    return E_NOTIMPL;
}

static const struct IActivationFactoryVtbl factory_vtbl =
{
    factory_QueryInterface,
    factory_AddRef,
    factory_Release,
    /* IInspectable methods */
    factory_GetIids,
    factory_GetRuntimeClassName,
    factory_GetTrustLevel,
    /* IActivationFactory methods */
    factory_ActivateInstance,
};

DEFINE_IINSPECTABLE( corewindow_static, ICoreWindowStatic, struct corewindow_statics, IActivationFactory_iface )

static HRESULT v55_corewindow_get_current(ICoreWindow **out);
static HRESULT STDMETHODCALLTYPE corewindow_static_GetForCurrentThread( ICoreWindowStatic *iface, ICoreWindow **windows )
{
    FIXME( "iface %p, windows %p stub!\n", iface, windows );

    if (!windows) return E_POINTER;
    *windows = NULL;
    return v55_corewindow_get_current(windows);
}

static const struct ICoreWindowStaticVtbl corewindow_static_vtbl =
{
    corewindow_static_QueryInterface,
    corewindow_static_AddRef,
    corewindow_static_Release,
    /* IInspectable methods */
    corewindow_static_GetIids,
    corewindow_static_GetRuntimeClassName,
    corewindow_static_GetTrustLevel,
    /* ICoreWindowStatic methods */
    corewindow_static_GetForCurrentThread
};

/* v55 EXPERIMENT — real thread-owned Win32 HWND and WinRT COM interfaces.
 * This covers a limited owned window + CoreDispatcher.ProcessEvents only.
 * It is not a production UWP CoreWindow implementation.
 * Private WineCoreWindowV55_Create is test-only and opt-in.
 */
struct corewindow_v55
{
    ICoreWindow ICoreWindow_iface;
    ICoreDispatcher ICoreDispatcher_iface;
    LONG refs;
    DWORD owner_tid;
    HWND hwnd;
    LONG quit;
};
static INIT_ONCE v55_init_once=INIT_ONCE_STATIC_INIT;
static DWORD v55_tls=TLS_OUT_OF_INDEXES;
static const WCHAR v55_class_name[]=L"WineExperimentalWinRTCoreWindowV55";

static struct corewindow_v55 *v55_from_window(ICoreWindow *iface)
{
    return CONTAINING_RECORD(iface,struct corewindow_v55,ICoreWindow_iface);
}
static struct corewindow_v55 *v55_from_dispatcher(ICoreDispatcher *iface)
{
    return CONTAINING_RECORD(iface,struct corewindow_v55,ICoreDispatcher_iface);
}
static BOOL v55_flag(void)
{
    char value[4];
    return GetEnvironmentVariableA("WINE_COREWINDOW_V55",value,sizeof(value))==1 && value[0]=='1';
}
static ULONG WINAPI v55_window_AddRef(ICoreWindow *iface)
{
    return InterlockedIncrement(&v55_from_window(iface)->refs);
}
static ULONG WINAPI v55_window_Release(ICoreWindow *iface)
{
    struct corewindow_v55 *obj=v55_from_window(iface);
    ULONG count=InterlockedDecrement(&obj->refs);
    if (!count) free(obj);
    return count;
}
static HRESULT WINAPI v55_window_QueryInterface(ICoreWindow *iface,REFIID iid,void **out)
{
    struct corewindow_v55 *obj=v55_from_window(iface);
    if (!out) return E_POINTER;
    *out=NULL;
    if (IsEqualGUID(iid,&IID_IUnknown) || IsEqualGUID(iid,&IID_IInspectable) ||
        IsEqualGUID(iid,&IID_ICoreWindow))
        *out=&obj->ICoreWindow_iface;
    else if (IsEqualGUID(iid,&IID_ICoreDispatcher))
        *out=&obj->ICoreDispatcher_iface;
    else
        return E_NOINTERFACE;
    v55_window_AddRef(&obj->ICoreWindow_iface);
    return S_OK;
}
static HRESULT WINAPI v55_window_GetIids(ICoreWindow *iface,ULONG *count,IID **iids)
{
    if (count) *count=0;
    if (iids) *iids=NULL;
    return E_NOTIMPL;
}
static HRESULT WINAPI v55_window_GetRuntimeClassName(ICoreWindow *iface,HSTRING *name)
{
    if (!name) return E_POINTER;
    *name=NULL;
    return WindowsCreateString(L"Windows.UI.Core.CoreWindow",26,name);
}
static HRESULT WINAPI v55_window_GetTrustLevel(ICoreWindow *iface,TrustLevel *level)
{
    if (!level) return E_POINTER;
    *level=BaseTrust;
    return S_OK;
}
static HRESULT WINAPI v55_window_get_Bounds(ICoreWindow *iface,Rect *value)
{
    struct corewindow_v55 *obj=v55_from_window(iface);
    RECT rect;
    if (!value) return E_POINTER;
    memset(value,0,sizeof(*value));
    if (GetCurrentThreadId()!=obj->owner_tid) return RPC_E_WRONG_THREAD;
    if (!obj->hwnd || !GetClientRect(obj->hwnd,&rect))
        return HRESULT_FROM_WIN32(ERROR_INVALID_WINDOW_HANDLE);
    value->X=0;
    value->Y=0;
    value->Width=(FLOAT)(rect.right-rect.left);
    value->Height=(FLOAT)(rect.bottom-rect.top);
    return S_OK;
}
static HRESULT WINAPI v55_window_get_Dispatcher(ICoreWindow *iface,ICoreDispatcher **out)
{
    struct corewindow_v55 *obj=v55_from_window(iface);
    if (!out) return E_POINTER;
    *out=NULL;
    if (GetCurrentThreadId()!=obj->owner_tid) return RPC_E_WRONG_THREAD;
    if (!obj->hwnd) return HRESULT_FROM_WIN32(ERROR_INVALID_WINDOW_HANDLE);
    v55_window_AddRef(iface);
    *out=&obj->ICoreDispatcher_iface;
    return S_OK;
}
static HRESULT WINAPI v55_window_get_IsInputEnabled(ICoreWindow *iface,boolean *out)
{
    if (!out) return E_POINTER;
    *out=FALSE;
    return E_NOTIMPL; /* requires actual WinRT input/event integration */
}
static HRESULT WINAPI v55_window_put_IsInputEnabled(ICoreWindow *iface,boolean value)
{
    return E_NOTIMPL; /* Do not imply actual input suppression. */
}
static HRESULT WINAPI v55_window_get_Visible(ICoreWindow *iface,boolean *out)
{
    struct corewindow_v55 *obj=v55_from_window(iface);
    if (!out) return E_POINTER;
    *out=FALSE;
    if (GetCurrentThreadId()!=obj->owner_tid) return RPC_E_WRONG_THREAD;
    if (!obj->hwnd) return HRESULT_FROM_WIN32(ERROR_INVALID_WINDOW_HANDLE);
    *out=IsWindowVisible(obj->hwnd) ? TRUE : FALSE;
    return S_OK;
}
static HRESULT WINAPI v55_window_Close(ICoreWindow *iface)
{
    struct corewindow_v55 *obj=v55_from_window(iface);
    if (GetCurrentThreadId()!=obj->owner_tid) return RPC_E_WRONG_THREAD;
    if (!obj->hwnd || !IsWindow(obj->hwnd))
        return HRESULT_FROM_WIN32(ERROR_INVALID_WINDOW_HANDLE);
    TRACE("COREWINDOW55_CLOSE hwnd=%p tid=%lu\n",obj->hwnd,obj->owner_tid);
    if (!DestroyWindow(obj->hwnd))
        return HRESULT_FROM_WIN32(GetLastError());
    return S_OK;
}
static HRESULT WINAPI v55_dispatcher_QueryInterface(ICoreDispatcher *iface,REFIID iid,void **out)
{
    struct corewindow_v55 *obj=v55_from_dispatcher(iface);
    return v55_window_QueryInterface(&obj->ICoreWindow_iface,iid,out);
}
static ULONG WINAPI v55_dispatcher_AddRef(ICoreDispatcher *iface)
{
    return v55_window_AddRef(&v55_from_dispatcher(iface)->ICoreWindow_iface);
}
static ULONG WINAPI v55_dispatcher_Release(ICoreDispatcher *iface)
{
    return v55_window_Release(&v55_from_dispatcher(iface)->ICoreWindow_iface);
}
static HRESULT WINAPI v55_dispatcher_GetIids(ICoreDispatcher *iface,ULONG *count,IID **iids)
{
    if (count) *count=0;
    if (iids) *iids=NULL;
    return E_NOTIMPL;
}
static HRESULT WINAPI v55_dispatcher_GetRuntimeClassName(ICoreDispatcher *iface,HSTRING *name)
{
    if (!name) return E_POINTER;
    *name=NULL;
    return WindowsCreateString(L"Windows.UI.Core.CoreDispatcher",30,name);
}
static HRESULT WINAPI v55_dispatcher_GetTrustLevel(ICoreDispatcher *iface,TrustLevel *level)
{
    if (!level) return E_POINTER;
    *level=BaseTrust;
    return S_OK;
}
static HRESULT WINAPI v55_dispatcher_get_HasThreadAccess(ICoreDispatcher *iface,boolean *value)
{
    struct corewindow_v55 *obj=v55_from_dispatcher(iface);
    if (!value) return E_POINTER;
    *value=(GetCurrentThreadId()==obj->owner_tid) ? TRUE : FALSE;
    return S_OK;
}
static BOOL v55_dispatch_one(struct corewindow_v55 *obj,MSG *message)
{
    if (message->message==WM_QUIT)
    {
        InterlockedExchange(&obj->quit,1);
        TRACE("COREDISPATCHER55_QUIT\n");
        return FALSE;
    }
    TranslateMessage(message);
    DispatchMessageW(message);
    return TRUE;
}
static HRESULT WINAPI v55_dispatcher_ProcessEvents(ICoreDispatcher *iface,CoreProcessEventsOption option)
{
    struct corewindow_v55 *obj=v55_from_dispatcher(iface);
    MSG msg;
    BOOL got;
    if (GetCurrentThreadId()!=obj->owner_tid) return RPC_E_WRONG_THREAD;
    if (option!=CoreProcessEventsOption_ProcessOneAndAllPending &&
        option!=CoreProcessEventsOption_ProcessOneIfPresent &&
        option!=CoreProcessEventsOption_ProcessUntilQuit &&
        option!=CoreProcessEventsOption_ProcessAllIfPresent)
        return E_INVALIDARG;
    if (!obj->hwnd || InterlockedCompareExchange(&obj->quit,0,0))
        return HRESULT_FROM_WIN32(ERROR_INVALID_WINDOW_HANDLE);
    TRACE("COREDISPATCHER55_PROCESS option=%d hwnd=%p\n",option,obj->hwnd);
    if (option==CoreProcessEventsOption_ProcessOneIfPresent ||
        option==CoreProcessEventsOption_ProcessAllIfPresent)
    {
        do
        {
            got=PeekMessageW(&msg,NULL,0,0,PM_REMOVE);
            if (!got) break;
            if (!v55_dispatch_one(obj,&msg)) break;
        } while (option==CoreProcessEventsOption_ProcessAllIfPresent);
        return S_OK;
    }
    if (option==CoreProcessEventsOption_ProcessOneAndAllPending)
    {
        int result=GetMessageW(&msg,NULL,0,0);
        if (result==-1) return HRESULT_FROM_WIN32(GetLastError());
        if (!result) { InterlockedExchange(&obj->quit,1); return S_OK; }
        v55_dispatch_one(obj,&msg);
        while (PeekMessageW(&msg,NULL,0,0,PM_REMOVE))
            if (!v55_dispatch_one(obj,&msg)) break;
        return S_OK;
    }
    for (;;)
    {
        int result=GetMessageW(&msg,NULL,0,0);
        if (result==-1) return HRESULT_FROM_WIN32(GetLastError());
        if (!result) { InterlockedExchange(&obj->quit,1); break; }
        if (!v55_dispatch_one(obj,&msg)) break;
        if (InterlockedCompareExchange(&obj->quit,0,0)) break;
    }
    TRACE("COREDISPATCHER55_UNTIL_QUIT_COMPLETE\n");
    return S_OK;
}
static HRESULT WINAPI v55_dispatcher_RunAsync(ICoreDispatcher *iface,
    CoreDispatcherPriority priority,IDispatchedHandler *handler,IAsyncAction **action)
{
    if (action) *action=NULL;
    FIXME("v55 RunAsync requires a completed asynchronous-action contract.\n");
    return E_NOTIMPL;
}
static HRESULT WINAPI v55_dispatcher_RunIdleAsync(ICoreDispatcher *iface,
    IIdleDispatchedHandler *handler,IAsyncAction **action)
{
    if (action) *action=NULL;
    FIXME("v55 RunIdleAsync requires a completed asynchronous-action contract.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_get_AutomationHostProvider(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, IInspectable **value)
{
    FIXME("v55 CoreWindow method get_AutomationHostProvider not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_get_CustomProperties(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, __x_ABI_CWindows_CFoundation_CCollections_CIPropertySet **value)
{
    FIXME("v55 CoreWindow method get_CustomProperties not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_get_FlowDirection(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, __x_ABI_CWindows_CUI_CCore_CCoreWindowFlowDirection *value)
{
    FIXME("v55 CoreWindow method get_FlowDirection not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_put_FlowDirection(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, __x_ABI_CWindows_CUI_CCore_CCoreWindowFlowDirection value)
{
    FIXME("v55 CoreWindow method put_FlowDirection not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_get_PointerCursor(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, __x_ABI_CWindows_CUI_CCore_CICoreCursor **value)
{
    FIXME("v55 CoreWindow method get_PointerCursor not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_put_PointerCursor(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, __x_ABI_CWindows_CUI_CCore_CICoreCursor *value)
{
    FIXME("v55 CoreWindow method put_PointerCursor not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_get_PointerPosition(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, __x_ABI_CWindows_CFoundation_CPoint *value)
{
    FIXME("v55 CoreWindow method get_PointerPosition not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_Activate(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This)
{
    FIXME("v55 CoreWindow method Activate not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_GetAsyncKeyState(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, __x_ABI_CWindows_CSystem_CVirtualKey key, __x_ABI_CWindows_CUI_CCore_CCoreVirtualKeyStates *state)
{
    FIXME("v55 CoreWindow method GetAsyncKeyState not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_GetKeyState(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, __x_ABI_CWindows_CSystem_CVirtualKey key, __x_ABI_CWindows_CUI_CCore_CCoreVirtualKeyStates *state)
{
    FIXME("v55 CoreWindow method GetKeyState not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_ReleasePointerCapture(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This)
{
    FIXME("v55 CoreWindow method ReleasePointerCapture not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_SetPointerCapture(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This)
{
    FIXME("v55 CoreWindow method SetPointerCapture not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_add_Activated(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CWindowActivatedEventArgs *handler, EventRegistrationToken *cookie)
{
    FIXME("v55 CoreWindow method add_Activated not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_remove_Activated(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, EventRegistrationToken cookie)
{
    FIXME("v55 CoreWindow method remove_Activated not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_add_AutomationProviderRequested(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CAutomationProviderRequestedEventArgs *handler, EventRegistrationToken *cookie)
{
    FIXME("v55 CoreWindow method add_AutomationProviderRequested not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_remove_AutomationProviderRequested(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, EventRegistrationToken cookie)
{
    FIXME("v55 CoreWindow method remove_AutomationProviderRequested not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_add_CharacterReceived(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CCharacterReceivedEventArgs *handler, EventRegistrationToken *cookie)
{
    FIXME("v55 CoreWindow method add_CharacterReceived not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_remove_CharacterReceived(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, EventRegistrationToken cookie)
{
    FIXME("v55 CoreWindow method remove_CharacterReceived not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_add_Closed(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CCoreWindowEventArgs *handler, EventRegistrationToken *cookie)
{
    FIXME("v55 CoreWindow method add_Closed not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_remove_Closed(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, EventRegistrationToken cookie)
{
    FIXME("v55 CoreWindow method remove_Closed not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_add_InputEnabled(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CInputEnabledEventArgs *handler, EventRegistrationToken *cookie)
{
    FIXME("v55 CoreWindow method add_InputEnabled not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_remove_InputEnabled(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, EventRegistrationToken cookie)
{
    FIXME("v55 CoreWindow method remove_InputEnabled not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_add_KeyDown(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CKeyEventArgs *handler, EventRegistrationToken *cookie)
{
    FIXME("v55 CoreWindow method add_KeyDown not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_remove_KeyDown(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, EventRegistrationToken cookie)
{
    FIXME("v55 CoreWindow method remove_KeyDown not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_add_KeyUp(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CKeyEventArgs *handler, EventRegistrationToken *cookie)
{
    FIXME("v55 CoreWindow method add_KeyUp not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_remove_KeyUp(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, EventRegistrationToken cookie)
{
    FIXME("v55 CoreWindow method remove_KeyUp not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_add_PointerCaptureLost(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CPointerEventArgs *handler, EventRegistrationToken *cookie)
{
    FIXME("v55 CoreWindow method add_PointerCaptureLost not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_remove_PointerCaptureLost(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, EventRegistrationToken cookie)
{
    FIXME("v55 CoreWindow method remove_PointerCaptureLost not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_add_PointerEntered(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CPointerEventArgs *handler, EventRegistrationToken *cookie)
{
    FIXME("v55 CoreWindow method add_PointerEntered not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_remove_PointerEntered(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, EventRegistrationToken cookie)
{
    FIXME("v55 CoreWindow method remove_PointerEntered not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_add_PointerExited(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CPointerEventArgs *handler, EventRegistrationToken *cookie)
{
    FIXME("v55 CoreWindow method add_PointerExited not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_remove_PointerExited(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, EventRegistrationToken cookie)
{
    FIXME("v55 CoreWindow method remove_PointerExited not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_add_PointerMoved(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CPointerEventArgs *handler, EventRegistrationToken *cookie)
{
    FIXME("v55 CoreWindow method add_PointerMoved not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_remove_PointerMoved(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, EventRegistrationToken cookie)
{
    FIXME("v55 CoreWindow method remove_PointerMoved not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_add_PointerPressed(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CPointerEventArgs *handler, EventRegistrationToken *cookie)
{
    FIXME("v55 CoreWindow method add_PointerPressed not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_remove_PointerPressed(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, EventRegistrationToken cookie)
{
    FIXME("v55 CoreWindow method remove_PointerPressed not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_add_PointerReleased(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CPointerEventArgs *handler, EventRegistrationToken *cookie)
{
    FIXME("v55 CoreWindow method add_PointerReleased not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_remove_PointerReleased(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, EventRegistrationToken cookie)
{
    FIXME("v55 CoreWindow method remove_PointerReleased not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_add_TouchHitTesting(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CTouchHitTestingEventArgs *handler, EventRegistrationToken *cookie)
{
    FIXME("v55 CoreWindow method add_TouchHitTesting not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_remove_TouchHitTesting(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, EventRegistrationToken cookie)
{
    FIXME("v55 CoreWindow method remove_TouchHitTesting not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_add_PointerWheelChanged(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CPointerEventArgs *handler, EventRegistrationToken *cookie)
{
    FIXME("v55 CoreWindow method add_PointerWheelChanged not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_remove_PointerWheelChanged(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, EventRegistrationToken cookie)
{
    FIXME("v55 CoreWindow method remove_PointerWheelChanged not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_add_SizeChanged(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CWindowSizeChangedEventArgs *handler, EventRegistrationToken *cookie)
{
    FIXME("v55 CoreWindow method add_SizeChanged not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_remove_SizeChanged(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, EventRegistrationToken cookie)
{
    FIXME("v55 CoreWindow method remove_SizeChanged not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_add_VisibilityChanged(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CVisibilityChangedEventArgs *handler, EventRegistrationToken *cookie)
{
    FIXME("v55 CoreWindow method add_VisibilityChanged not implemented.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI v55_window_remove_VisibilityChanged(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, EventRegistrationToken cookie)
{
    FIXME("v55 CoreWindow method remove_VisibilityChanged not implemented.\n");
    return E_NOTIMPL;
}


static const ICoreDispatcherVtbl v55_dispatcher_vtbl =
{
    .QueryInterface=v55_dispatcher_QueryInterface,
    .AddRef=v55_dispatcher_AddRef,
    .Release=v55_dispatcher_Release,
    .GetIids=v55_dispatcher_GetIids,
    .GetRuntimeClassName=v55_dispatcher_GetRuntimeClassName,
    .GetTrustLevel=v55_dispatcher_GetTrustLevel,
    .get_HasThreadAccess=v55_dispatcher_get_HasThreadAccess,
    .ProcessEvents=v55_dispatcher_ProcessEvents,
    .RunAsync=v55_dispatcher_RunAsync,
    .RunIdleAsync=v55_dispatcher_RunIdleAsync
};
static const ICoreWindowVtbl v55_window_vtbl =
{
    .QueryInterface=v55_window_QueryInterface,
    .AddRef=v55_window_AddRef,
    .Release=v55_window_Release,
    .GetIids=v55_window_GetIids,
    .GetRuntimeClassName=v55_window_GetRuntimeClassName,
    .GetTrustLevel=v55_window_GetTrustLevel,
    .get_AutomationHostProvider=v55_window_get_AutomationHostProvider,
    .get_Bounds=v55_window_get_Bounds,
    .get_CustomProperties=v55_window_get_CustomProperties,
    .get_Dispatcher=v55_window_get_Dispatcher,
    .get_FlowDirection=v55_window_get_FlowDirection,
    .put_FlowDirection=v55_window_put_FlowDirection,
    .get_IsInputEnabled=v55_window_get_IsInputEnabled,
    .put_IsInputEnabled=v55_window_put_IsInputEnabled,
    .get_PointerCursor=v55_window_get_PointerCursor,
    .put_PointerCursor=v55_window_put_PointerCursor,
    .get_PointerPosition=v55_window_get_PointerPosition,
    .get_Visible=v55_window_get_Visible,
    .Activate=v55_window_Activate,
    .Close=v55_window_Close,
    .GetAsyncKeyState=v55_window_GetAsyncKeyState,
    .GetKeyState=v55_window_GetKeyState,
    .ReleasePointerCapture=v55_window_ReleasePointerCapture,
    .SetPointerCapture=v55_window_SetPointerCapture,
    .add_Activated=v55_window_add_Activated,
    .remove_Activated=v55_window_remove_Activated,
    .add_AutomationProviderRequested=v55_window_add_AutomationProviderRequested,
    .remove_AutomationProviderRequested=v55_window_remove_AutomationProviderRequested,
    .add_CharacterReceived=v55_window_add_CharacterReceived,
    .remove_CharacterReceived=v55_window_remove_CharacterReceived,
    .add_Closed=v55_window_add_Closed,
    .remove_Closed=v55_window_remove_Closed,
    .add_InputEnabled=v55_window_add_InputEnabled,
    .remove_InputEnabled=v55_window_remove_InputEnabled,
    .add_KeyDown=v55_window_add_KeyDown,
    .remove_KeyDown=v55_window_remove_KeyDown,
    .add_KeyUp=v55_window_add_KeyUp,
    .remove_KeyUp=v55_window_remove_KeyUp,
    .add_PointerCaptureLost=v55_window_add_PointerCaptureLost,
    .remove_PointerCaptureLost=v55_window_remove_PointerCaptureLost,
    .add_PointerEntered=v55_window_add_PointerEntered,
    .remove_PointerEntered=v55_window_remove_PointerEntered,
    .add_PointerExited=v55_window_add_PointerExited,
    .remove_PointerExited=v55_window_remove_PointerExited,
    .add_PointerMoved=v55_window_add_PointerMoved,
    .remove_PointerMoved=v55_window_remove_PointerMoved,
    .add_PointerPressed=v55_window_add_PointerPressed,
    .remove_PointerPressed=v55_window_remove_PointerPressed,
    .add_PointerReleased=v55_window_add_PointerReleased,
    .remove_PointerReleased=v55_window_remove_PointerReleased,
    .add_TouchHitTesting=v55_window_add_TouchHitTesting,
    .remove_TouchHitTesting=v55_window_remove_TouchHitTesting,
    .add_PointerWheelChanged=v55_window_add_PointerWheelChanged,
    .remove_PointerWheelChanged=v55_window_remove_PointerWheelChanged,
    .add_SizeChanged=v55_window_add_SizeChanged,
    .remove_SizeChanged=v55_window_remove_SizeChanged,
    .add_VisibilityChanged=v55_window_add_VisibilityChanged,
    .remove_VisibilityChanged=v55_window_remove_VisibilityChanged
};
static LRESULT CALLBACK v55_wndproc(HWND hwnd,UINT message,WPARAM wp,LPARAM lp)
{
    struct corewindow_v55 *obj=(struct corewindow_v55 *)GetWindowLongPtrW(hwnd,GWLP_USERDATA);
    switch(message)
    {
    case WM_APP+55:
        TRACE("COREWINDOW55_APP_MESSAGE_RECEIVED hwnd=%p\n",hwnd);
        return 0;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_NCDESTROY:
        if (obj)
        {
            SetWindowLongPtrW(hwnd,GWLP_USERDATA,0);
            obj->hwnd=NULL;
            if (v55_tls!=TLS_OUT_OF_INDEXES && TlsGetValue(v55_tls)==obj)
                TlsSetValue(v55_tls,NULL);
            v55_window_Release(&obj->ICoreWindow_iface);
        }
        return DefWindowProcW(hwnd,message,wp,lp);
    default:
        return DefWindowProcW(hwnd,message,wp,lp);
    }
}

static HRESULT v55_corewindow_get_current(ICoreWindow **out)
{
    struct corewindow_v55 *obj;
    if (!v55_flag() || v55_tls==TLS_OUT_OF_INDEXES) return S_OK;
    obj=TlsGetValue(v55_tls);
    if (obj && obj->hwnd && IsWindow(obj->hwnd))
    {
        *out=&obj->ICoreWindow_iface;
        v55_window_AddRef(*out);
        TRACE("COREWINDOW55_FOR_CURRENT_THREAD hwnd=%p thread=%lu\n",obj->hwnd,obj->owner_tid);
    }
    return S_OK;
}

static BOOL CALLBACK v55_init(INIT_ONCE *once,void *parameter,void **context)
{
    WNDCLASSEXW wc;
    DWORD tls=TlsAlloc();
    if (tls==TLS_OUT_OF_INDEXES) return FALSE;
    memset(&wc,0,sizeof(wc));
    wc.cbSize=sizeof(wc);
    wc.hInstance=GetModuleHandleW(L"windows.ui.dll");
    if (!wc.hInstance) wc.hInstance=GetModuleHandleW(NULL);
    wc.lpfnWndProc=v55_wndproc;
    wc.lpszClassName=v55_class_name;
    if (!RegisterClassExW(&wc) && GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)
    {
        TlsFree(tls);
        return FALSE;
    }
    v55_tls=tls;
    return TRUE;
}
/* Experimental private entry point; never treated as a native Windows API. */
HRESULT WINAPI WineCoreWindowV55_Create(ICoreWindow **window,HWND *hwnd)
{
    struct corewindow_v55 *obj;
    HWND created;
    if (!window || !hwnd) return E_POINTER;
    *window=NULL;
    *hwnd=NULL;
    if (!v55_flag()) return E_NOTIMPL;
    if (!InitOnceExecuteOnce(&v55_init_once,v55_init,NULL,NULL))
        return HRESULT_FROM_WIN32(GetLastError());
    if (TlsGetValue(v55_tls)) return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
    if (!(obj=calloc(1,sizeof(*obj)))) return E_OUTOFMEMORY;
    obj->ICoreWindow_iface.lpVtbl=&v55_window_vtbl;
    obj->ICoreDispatcher_iface.lpVtbl=&v55_dispatcher_vtbl;
    obj->refs=1; /* caller */
    obj->owner_tid=GetCurrentThreadId();
    created=CreateWindowExW(0,v55_class_name,L"Wine WinRT v55 experimental CoreWindow",
        WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,640,480,
        NULL,NULL,GetModuleHandleW(L"windows.ui.dll"),NULL);
    if (!created)
    {
        HRESULT hr=HRESULT_FROM_WIN32(GetLastError());
        v55_window_Release(&obj->ICoreWindow_iface);
        return hr;
    }
    obj->hwnd=created;
    v55_window_AddRef(&obj->ICoreWindow_iface); /* HWND lifetime */
    SetWindowLongPtrW(created,GWLP_USERDATA,(LONG_PTR)obj);
    if (!TlsSetValue(v55_tls,obj))
    {
        HRESULT hr=HRESULT_FROM_WIN32(GetLastError());
        DestroyWindow(created);
        v55_window_Release(&obj->ICoreWindow_iface);
        return hr;
    }
    *window=&obj->ICoreWindow_iface;
    *hwnd=created;
    TRACE("COREWINDOW55_CREATED hwnd=%p thread=%lu object=%p\n",created,obj->owner_tid,obj);
    return S_OK;
}

static struct corewindow_statics corewindow_statics =
{
    {&factory_vtbl},
    {&corewindow_static_vtbl},
    1,
};

IActivationFactory *corewindow_factory = &corewindow_statics.IActivationFactory_iface;
