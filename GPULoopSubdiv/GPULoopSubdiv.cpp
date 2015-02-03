// Written by P.Wu, email: pxiangwu@gmail.com

#include "DXUT.h"
#include "DXUTcamera.h"
#include "DXUTgui.h"
#include "DXUTsettingsDlg.h"
#include "SDKmisc.h"
#include "resource.h"
#include "MainRenderer.h"

const DWORD MIN_DIVS = 0;
const DWORD MAX_DIVS = 7; // Min and Max divisions of the patch per side for the slider control

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
CDXUTDialogResourceManager          g_DialogResourceManager; // manager for shared resources of dialogs
CModelViewerCamera                  g_Camera;                // A model viewing camera
CD3DSettingsDlg                     g_D3DSettingsDlg;        // Device settings dialog
CDXUTDialog                         g_HUD;                   // manages the 3D   
CDXUTDialog                         g_SampleUI;              // dialog for sample specific controls

// Resources
CDXUTTextHelper*                    g_pTxtHelper = NULL;

//ID3D11InputLayout*                  g_pPatchLayout = NULL;
//
//ID3D11VertexShader*                 g_pVertexShader = NULL;
//ID3D11HullShader*                   g_pHullShaderInteger = NULL;
//ID3D11HullShader*                   g_pHullShaderFracEven = NULL;
//ID3D11HullShader*                   g_pHullShaderFracOdd = NULL;
//ID3D11DomainShader*                 g_pDomainShader = NULL;
//ID3D11PixelShader*                  g_pPixelShader = NULL;
//ID3D11PixelShader*                  g_pSolidColorPS = NULL;

//ID3D11Buffer*   g_pControlPointVB;                           // Control points for mesh

struct CB_PER_FRAME_CONSTANTS
{
    D3DXMATRIX mViewProjection;
    D3DXVECTOR3 vCameraPosWorld;
	float dummy;
};

ID3D11Buffer*                       g_pcbPerFrame = NULL;
UINT                                g_iBindPerFrame = 0;

ID3D11RasterizerState*              ControlMesh::s_pRasterizerStateSolid = NULL;
ID3D11RasterizerState*              ControlMesh::s_pRasterizerStateWireframe = NULL;
ID3D11Buffer*					ControlMesh::s_pConstantBufferPerLevel = NULL;

// Control variables
//float                               ControlMesh::m_fTessFactor = 1;          // Startup subdivisions per side
bool                                g_bDrawWires = false;    // Draw the mesh with wireframe overlay
float ControlMesh::m_fTessFactor = 3.1f;

static bool g_bShowWireFrame = true;

enum E_PARTITION_MODE
{
   PARTITION_INTEGER,
   PARTITION_FRACTIONAL_EVEN,
   PARTITION_FRACTIONAL_ODD
};

E_PARTITION_MODE                    g_iPartitionMode = PARTITION_INTEGER;

//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN      1
#define IDC_TOGGLEREF             3
#define IDC_CHANGEDEVICE          4

#define IDC_PATCH_SUBDIVS         5
#define IDC_PATCH_SUBDIVS_STATIC  6
#define IDC_TOGGLE_LINES          7
#define IDC_PARTITION_MODE        8
#define IDC_PARTITION_INTEGER     9
#define IDC_PARTITION_FRAC_EVEN   10
#define IDC_PARTITION_FRAC_ODD    11
#define IDC_SHOWWIRE 12

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext );
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext );
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );

bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext );
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                      void* pUserContext );
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                          const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext );
void CALLBACK OnD3D11DestroyDevice( void* pUserContext );
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
                                  float fElapsedTime, void* pUserContext );


void InitApp();
void RenderText();

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // DXUT will create and use the best device (either D3D10 or D3D11) 
    // that is available on the system depending on which D3D callbacks are set below

    // Set DXUT callbacks
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackFrameMove( OnFrameMove );

    DXUTSetCallbackD3D11DeviceAcceptable( IsD3D11DeviceAcceptable );
    DXUTSetCallbackD3D11DeviceCreated( OnD3D11CreateDevice );
    DXUTSetCallbackD3D11SwapChainResized( OnD3D11ResizedSwapChain );
    DXUTSetCallbackD3D11FrameRender( OnD3D11FrameRender );
    DXUTSetCallbackD3D11SwapChainReleasing( OnD3D11ReleasingSwapChain );
    DXUTSetCallbackD3D11DeviceDestroyed( OnD3D11DestroyDevice );

    InitApp();
    DXUTInit( true, true ); // Parse the command line, show msgboxes on error, and an extra cmd line param to force REF for now
    DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen
    DXUTCreateWindow( L"Feature Adaptive GPU Loop Subdivision" );
    DXUTCreateDevice( D3D_FEATURE_LEVEL_11_0,  true, 640, 480 );
    DXUTMainLoop(); // Enter into the DXUT render loop

    return DXUTGetExitCode();
}


//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
    // Initialize dialogs
    g_D3DSettingsDlg.Init( &g_DialogResourceManager );
    g_HUD.Init( &g_DialogResourceManager );
    g_SampleUI.Init( &g_DialogResourceManager );

    g_HUD.SetCallback( OnGUIEvent ); int iY = 20;
    g_HUD.AddButton( IDC_TOGGLEFULLSCREEN, L"Toggle full screen",0, iY, 170, 22 );
    g_HUD.AddButton( IDC_TOGGLEREF, L"Toggle REF (F3)", 0, iY += 26, 170, 22, VK_F3 );
    g_HUD.AddButton( IDC_CHANGEDEVICE, L"Change device (F2)", 0, iY += 26, 170, 22, VK_F2 );

    g_SampleUI.SetCallback( OnGUIEvent ); iY = 10;

    WCHAR sz[100];
    iY += 24;
    swprintf_s( sz, L"Patch Divisions: %2.1f", ControlMesh::m_fTessFactor );
    g_SampleUI.AddStatic( IDC_PATCH_SUBDIVS_STATIC, sz, 10, iY += 26, 150, 22 );
    g_SampleUI.AddSlider( IDC_PATCH_SUBDIVS, 10, iY += 24, 150, 22, 10 * MIN_DIVS, 10 * MAX_DIVS, (int)(ControlMesh::m_fTessFactor * 10) );
    iY += 24;
    //g_SampleUI.AddCheckBox( IDC_TOGGLE_LINES, L"Toggle Wires", 20, iY += 26, 150, 22, g_bDrawWires );
	g_SampleUI.AddCheckBox( IDC_SHOWWIRE, L"Show Wireframe", 20, iY += 26, 150, 22, g_bShowWireFrame, (UINT)'W');
    //iY += 24;
    //g_SampleUI.AddRadioButton( IDC_PARTITION_INTEGER, IDC_PARTITION_MODE, L"Integer", 20, iY += 26, 170, 22 );
    //g_SampleUI.AddRadioButton( IDC_PARTITION_FRAC_EVEN, IDC_PARTITION_MODE, L"Fractional Even", 20, iY += 26, 170, 22 );
    //g_SampleUI.AddRadioButton( IDC_PARTITION_FRAC_ODD, IDC_PARTITION_MODE, L"Fractional Odd", 20, iY += 26, 170, 22 );
    //g_SampleUI.GetRadioButton( IDC_PARTITION_INTEGER )->SetChecked( true );

    // Setup the camera's view parameters
    D3DXVECTOR3 vecEye( 1.0f, 1.5f, -3.5f );
    D3DXVECTOR3 vecAt ( 0.0f, 0.0f, 0.0f );
    g_Camera.SetViewParams( &vecEye, &vecAt );

}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D10 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    // For the first device created if its a REF device, optionally display a warning dialog box
    static bool s_bFirstTime = true;
    if( s_bFirstTime )
    {
        s_bFirstTime = false;
        if( ( DXUT_D3D9_DEVICE == pDeviceSettings->ver && pDeviceSettings->d3d9.DeviceType == D3DDEVTYPE_REF ) ||
            ( DXUT_D3D11_DEVICE == pDeviceSettings->ver &&
              pDeviceSettings->d3d11.DriverType == D3D_DRIVER_TYPE_REFERENCE ) )
        {
            DXUTDisplaySwitchingToREFWarning( pDeviceSettings->ver );
        }
    }

    return true;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
    // Update the camera's position based on user input 
    g_Camera.FrameMove( fElapsedTime );
}


//--------------------------------------------------------------------------------------
// Render the help and statistics text
//--------------------------------------------------------------------------------------
void RenderText()
{
    g_pTxtHelper->Begin();
    g_pTxtHelper->SetInsertionPos( 2, 0 );
    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
    g_pTxtHelper->DrawTextLine( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
    g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );

    g_pTxtHelper->End();
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext )
{
    // Pass messages to dialog resource manager calls so GUI state is updated correctly
    *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass messages to settings dialog if its active
    if( g_D3DSettingsDlg.IsActive() )
    {
        g_D3DSettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
        return 0;
    }

    // Give the dialogs a chance to handle the message first
    *pbNoFurtherProcessing = g_HUD.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;
    *pbNoFurtherProcessing = g_SampleUI.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass all remaining windows messages to camera so it can respond to user input
    g_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );

    return 0;
}

//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
    switch( nControlID )
    {
            // Standard DXUT controls
        case IDC_TOGGLEFULLSCREEN:
            DXUTToggleFullScreen(); break;
        case IDC_TOGGLEREF:
            DXUTToggleREF(); break;
        case IDC_CHANGEDEVICE:
            g_D3DSettingsDlg.SetActive( !g_D3DSettingsDlg.IsActive() ); break;

            // Custom app controls
        case IDC_PATCH_SUBDIVS:
        {
            ControlMesh::m_fTessFactor = g_SampleUI.GetSlider( IDC_PATCH_SUBDIVS )->GetValue() / 10.0f;

            WCHAR sz[100];
            swprintf_s( sz, L"Patch Divisions: %2.1f", ControlMesh::m_fTessFactor );
            g_SampleUI.GetStatic( IDC_PATCH_SUBDIVS_STATIC )->SetText( sz );
        }
            break;
        case IDC_SHOWWIRE:
            g_bShowWireFrame = g_SampleUI.GetCheckBox(  IDC_SHOWWIRE )->GetChecked();
            break;
        case IDC_PARTITION_INTEGER:
            g_iPartitionMode = PARTITION_INTEGER;
            break;
        case IDC_PARTITION_FRAC_EVEN:
            g_iPartitionMode = PARTITION_FRACTIONAL_EVEN;
            break;
        case IDC_PARTITION_FRAC_ODD:
            g_iPartitionMode = PARTITION_FRACTIONAL_ODD;
            break;
    }
}

//--------------------------------------------------------------------------------------
// Reject any D3D11 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    return true;
}

//--------------------------------------------------------------------------------------
// Find and compile the specified shader
//--------------------------------------------------------------------------------------
HRESULT CompileShaderFromFile( WCHAR* szFileName, D3D_SHADER_MACRO* pDefines, LPCSTR szEntryPoint, 
                               LPCSTR szShaderModel, ID3DBlob** ppBlobOut )
{
    HRESULT hr = S_OK;

    // find the file
    WCHAR str[MAX_PATH];
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, szFileName ) );

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;
	//dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ID3DBlob* pErrorBlob;
    hr = D3DX11CompileFromFile( str, pDefines, NULL, szEntryPoint, szShaderModel, 
        dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL );
    if( FAILED(hr) )
    {
        if( pErrorBlob != NULL )
            OutputDebugStringA( (char*)pErrorBlob->GetBufferPointer() );
        SAFE_RELEASE( pErrorBlob );
        return hr;
    }
    SAFE_RELEASE( pErrorBlob );

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                      void* pUserContext )
{
    HRESULT hr;

    ID3D11DeviceContext* pd3dImmediateContext = DXUTGetD3D11DeviceContext();
    V_RETURN( g_DialogResourceManager.OnD3D11CreateDevice( pd3dDevice, pd3dImmediateContext ) );
    V_RETURN( g_D3DSettingsDlg.OnD3D11CreateDevice( pd3dDevice ) );
    g_pTxtHelper = new CDXUTTextHelper( pd3dDevice, pd3dImmediateContext, &g_DialogResourceManager, 15 );

	V_RETURN(OnCreateDevice(pd3dDevice, pd3dImmediateContext));

    // Create constant buffers
    D3D11_BUFFER_DESC Desc;
    Desc.Usage = D3D11_USAGE_DYNAMIC;
    Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Desc.MiscFlags = 0;

    Desc.ByteWidth = sizeof( CB_PER_FRAME_CONSTANTS );
    V_RETURN( pd3dDevice->CreateBuffer( &Desc, NULL, &g_pcbPerFrame ) );
    DXUT_SetDebugName( g_pcbPerFrame, "CB_PER_FRAME_CONSTANTS" );

	Desc.ByteWidth = sizeof(CB_PER_LEVEL_CONSTANTS);
	V_RETURN( pd3dDevice->CreateBuffer( &Desc, NULL, &ControlMesh::s_pConstantBufferPerLevel ) );
    DXUT_SetDebugName( ControlMesh::s_pConstantBufferPerLevel, "CB_PER_LEVEL_CONSTANTS" );

    // Create solid and wireframe rasterizer state objects
    D3D11_RASTERIZER_DESC RasterDesc;
    ZeroMemory( &RasterDesc, sizeof(D3D11_RASTERIZER_DESC) );
	RasterDesc.AntialiasedLineEnable = true;
    RasterDesc.FillMode = D3D11_FILL_SOLID;
    RasterDesc.CullMode = D3D11_CULL_NONE;
    RasterDesc.DepthClipEnable = TRUE;
    V_RETURN( pd3dDevice->CreateRasterizerState( &RasterDesc, &ControlMesh::s_pRasterizerStateSolid ) );
    DXUT_SetDebugName( ControlMesh::s_pRasterizerStateSolid, "Solid" );

    RasterDesc.FillMode = D3D11_FILL_WIREFRAME;
    V_RETURN( pd3dDevice->CreateRasterizerState( &RasterDesc, &ControlMesh::s_pRasterizerStateWireframe ) );
    DXUT_SetDebugName( ControlMesh::s_pRasterizerStateWireframe, "Wireframe" );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                          const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;

    V_RETURN( g_DialogResourceManager.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );
    V_RETURN( g_D3DSettingsDlg.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );

    // Setup the camera's projection parameters
    float fAspectRatio = pBackBufferSurfaceDesc->Width / ( FLOAT )pBackBufferSurfaceDesc->Height;
    g_Camera.SetProjParams( D3DX_PI / 4, fAspectRatio, 0.1f, 20.0f );
    g_Camera.SetWindow( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );
    g_Camera.SetButtonMasks( MOUSE_MIDDLE_BUTTON, MOUSE_WHEEL, MOUSE_LEFT_BUTTON );

    g_HUD.SetLocation( pBackBufferSurfaceDesc->Width - 170, 0 );
    g_HUD.SetSize( 170, 170 );
    g_SampleUI.SetLocation( pBackBufferSurfaceDesc->Width - 170, pBackBufferSurfaceDesc->Height - 300 );
    g_SampleUI.SetSize( 170, 300 );

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
                                  float fElapsedTime, void* pUserContext )
{
    // If the settings dialog is being shown, then render it instead of rendering the app's scene
    if( g_D3DSettingsDlg.IsActive() )
    {
        g_D3DSettingsDlg.OnRender( fElapsedTime );
        return;
    }

    // WVP
    D3DXMATRIX mViewProjection;
    D3DXMATRIX mProj = *g_Camera.GetProjMatrix();
    D3DXMATRIX mView = *g_Camera.GetViewMatrix();

    mViewProjection = mView * mProj;

    // Update per-frame variables
    D3D11_MAPPED_SUBRESOURCE MappedResource;
    pd3dImmediateContext->Map( g_pcbPerFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource );
    CB_PER_FRAME_CONSTANTS* pData = ( CB_PER_FRAME_CONSTANTS* )MappedResource.pData;

    D3DXMatrixTranspose( &pData->mViewProjection, &mViewProjection );
    pData->vCameraPosWorld = *( g_Camera.GetEyePt() );
    //pData->fTessellationFactor = (float)ControlMesh::m_fTessFactor;

    pd3dImmediateContext->Unmap( g_pcbPerFrame, 0 );

    // Clear the render target and depth stencil
    float ClearColor[4] = { 0.05f, 0.05f, 0.05f, 0.0f };
    pd3dImmediateContext->ClearRenderTargetView( DXUTGetD3D11RenderTargetView(), ClearColor );
    pd3dImmediateContext->ClearDepthStencilView( DXUTGetD3D11DepthStencilView(), D3D11_CLEAR_DEPTH, 1.0, 0 );

    // Set state for solid rendering
    pd3dImmediateContext->RSSetState( ControlMesh::s_pRasterizerStateSolid );

    // Render the meshes
    // Bind all of the CBs
    pd3dImmediateContext->VSSetConstantBuffers( g_iBindPerFrame, 1, &g_pcbPerFrame );
    pd3dImmediateContext->HSSetConstantBuffers( g_iBindPerFrame, 1, &g_pcbPerFrame );
    pd3dImmediateContext->DSSetConstantBuffers( g_iBindPerFrame, 1, &g_pcbPerFrame );
    pd3dImmediateContext->PSSetConstantBuffers( g_iBindPerFrame, 1, &g_pcbPerFrame );

    

    // Optionally draw the wireframe
    //if( g_bDrawWires )
    {
        //pd3dImmediateContext->PSSetShader( ControlMesh::s_pPixelShaderGreen, NULL, 0 );
        pd3dImmediateContext->RSSetState( ControlMesh::s_pRasterizerStateWireframe ); 
    }
	

	if (g_bShowWireFrame)
	{
		ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
		ID3D11DepthStencilView* pDSV = DXUTGetD3D11DepthStencilView();
		ID3D11RenderTargetView* nullRTV[1] = {NULL};
		pd3dImmediateContext->OMSetRenderTargets(1, nullRTV, pDSV);
		pd3dImmediateContext->RSSetState( ControlMesh::s_pRasterizerStateSolid );
		cmesh.FrameRender(pd3dDevice, pd3dImmediateContext,floor(ControlMesh::m_fTessFactor));

		pd3dImmediateContext->OMSetRenderTargets(1, &pRTV, pDSV);
		pd3dImmediateContext->RSSetState( ControlMesh::s_pRasterizerStateWireframe );// draw
		cmesh.FrameRender(pd3dDevice, pd3dImmediateContext,floor(ControlMesh::m_fTessFactor));
	}
	else
	{
		pd3dImmediateContext->RSSetState( ControlMesh::s_pRasterizerStateSolid );
		cmesh.FrameRender(pd3dDevice, pd3dImmediateContext,floor(ControlMesh::m_fTessFactor));
	}


	ID3D11ShaderResourceView* ppSRVNULL[16] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
	pd3dImmediateContext->VSSetShaderResources(0, 16, ppSRVNULL );
	pd3dImmediateContext->HSSetShaderResources(0, 16, ppSRVNULL );
	pd3dImmediateContext->DSSetShaderResources(0, 16, ppSRVNULL );
	pd3dImmediateContext->GSSetShaderResources(0, 16, ppSRVNULL );
	pd3dImmediateContext->PSSetShaderResources(0, 16, ppSRVNULL );
	pd3dImmediateContext->IASetIndexBuffer(NULL, DXGI_FORMAT_R32_UINT, 0);
	pd3dImmediateContext->IASetVertexBuffers(NULL, 0, NULL, NULL, NULL);

	pd3dImmediateContext->VSSetShader(NULL, NULL, 0);
	pd3dImmediateContext->HSSetShader(NULL, NULL, 0);
	pd3dImmediateContext->DSSetShader(NULL, NULL, 0);
	pd3dImmediateContext->GSSetShader(NULL, NULL, 0);
	pd3dImmediateContext->PSSetShader(NULL, NULL, 0);
    // Render the HUD
    DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"HUD / Stats" );
    g_HUD.OnRender( fElapsedTime );
    g_SampleUI.OnRender( fElapsedTime );
    RenderText();
    DXUT_EndPerfEvent();
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D10ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11ReleasingSwapChain();
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D10CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11DestroyDevice();
    g_D3DSettingsDlg.OnD3D11DestroyDevice();
    DXUTGetGlobalResourceCache().OnDestroyDevice();
    SAFE_DELETE( g_pTxtHelper );
    SAFE_RELEASE( g_pcbPerFrame );

	cmesh.DestroyShaders();
	cmesh.Destroy();
}
