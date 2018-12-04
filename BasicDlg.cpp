// BasicDlg.cpp : implementation file
//
//BIG5 TRANS ALLOWED

#include "stdafx.h"
#include "Basic.h"

#include "BasicDlg.h"
#include "malloc.h"

#include <opencv2/opencv.hpp>
#include<vector>

#include <process.h>
#include<time.h>
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern int gLanguage;
using namespace std;
using namespace cv;
#ifdef _WIN64
#pragma comment(lib, "..\\MVCAMSDK_X64.lib")
#else
#pragma comment(lib, "..\\MVCAMSDK.lib")
#endif
#include "..//include//CameraApi.h"

struct HoughParam
{
	int param1;
	int param2 ;
	int minRadius;
	int maxRadius;
};
HoughParam m_HoughBasic,m_HoughInput;
/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

/*������ô��ڵ���Ϣ�ص�����
hCamera:��ǰ����ľ��
MSG:��Ϣ���ͣ�
	SHEET_MSG_LOAD_PARAM_DEFAULT	= 0,//����Ĭ�ϲ����İ�ť�����������Ĭ�ϲ�����ɺ󴥷�����Ϣ,
	SHEET_MSG_LOAD_PARAM_GROUP		= 1,//�л���������ɺ󴥷�����Ϣ,
	SHEET_MSG_LOAD_PARAM_FROMFILE	= 2,//���ز�����ť��������Ѵ��ļ��м�����������󴥷�����Ϣ
	SHEET_MSG_SAVE_PARAM_GROUP		= 3//���������ť���������������󴥷�����Ϣ
	����μ�CameraDefine.h��emSdkPropSheetMsg����

uParam:��Ϣ�����Ĳ�������ͬ����Ϣ���������岻ͬ��
	�� MSG Ϊ SHEET_MSG_LOAD_PARAM_DEFAULTʱ��uParam��ʾ�����س�Ĭ�ϲ�����������ţ���0��ʼ���ֱ��ӦA,B,C,D����
	�� MSG Ϊ SHEET_MSG_LOAD_PARAM_GROUPʱ��uParam��ʾ�л���Ĳ�����������ţ���0��ʼ���ֱ��ӦA,B,C,D����
	�� MSG Ϊ SHEET_MSG_LOAD_PARAM_FROMFILEʱ��uParam��ʾ���ļ��в������ǵĲ�����������ţ���0��ʼ���ֱ��ӦA,B,C,D����
	�� MSG Ϊ SHEET_MSG_SAVE_PARAM_GROUPʱ��uParam��ʾ��ǰ����Ĳ�����������ţ���0��ʼ���ֱ��ӦA,B,C,D����
*/
void _stdcall CameraSettingPageCallback(CameraHandle hCamera,UINT MSG,UINT uParam,PVOID pContext)
{
	CBasicDlg *pDialog = (CBasicDlg*)pContext;
	CString sMsg;

	if (MSG == SHEET_MSG_LOAD_PARAM_DEFAULT)
	{
		sMsg.Format("Parameter group[%d] was loaded to default!",uParam);
	}
	else if (MSG == SHEET_MSG_LOAD_PARAM_GROUP)
	{
		sMsg.Format("Parameter group[%d] was loaded!",uParam);
	}
	else if (MSG == SHEET_MSG_LOAD_PARAM_FROMFILE)
	{
		sMsg.Format("Parameter group[%d] was loaded from file!",uParam);
	}
	else if (MSG == SHEET_MSG_SAVE_PARAM_GROUP)
	{
		sMsg.Format("Parameter group[%d] was saved!",uParam);
	}
	else
	{
		return;//unknown message type
	}
	pDialog->m_DlgLog.AppendLog("CallBack: CameraSettingPageCallback");
	pDialog->m_DlgLog.AppendLog(sMsg);
	pDialog->m_DlgLog.AppendLog(" ");

}

/*
USE_CALLBACK_GRAB_IMAGE
�����Ҫʹ�ûص������ķ�ʽ���ͼ�����ݣ���ע�ͺ궨��USE_CALLBACK_GRAB_IMAGE.
���ǵ�SDKͬʱ֧�ֻص��������������ýӿ�ץȡͼ��ķ�ʽ�����ַ�ʽ��������"�㿽��"���ƣ������ĳ̶ȵĽ���ϵͳ���ɣ���߳���ִ��Ч�ʡ�
��������ץȡ��ʽ�Ȼص������ķ�ʽ�������������ó�ʱ�ȴ�ʱ��ȣ����ǽ�����ʹ�� uiDisplayThread �еķ�ʽ
*/
//#define USE_CALLBACK_GRAB_IMAGE 

#ifdef USE_CALLBACK_GRAB_IMAGE
/*ͼ��ץȡ�ص�����*/
void _stdcall GrabImageCallback(CameraHandle hCamera, BYTE *pFrameBuffer, tSdkFrameHead* pFrameHead,PVOID pContext)
{
	
	CameraSdkStatus status;
	CBasicDlg *pThis = (CBasicDlg*)pContext;
	
	//����õ�ԭʼ����ת����RGB��ʽ�����ݣ�ͬʱ����ISPģ�飬��ͼ����н��룬������������ɫУ���ȴ���
	//�ҹ�˾�󲿷��ͺŵ������ԭʼ���ݶ���Bayer��ʽ��
	status = CameraImageProcess(pThis->m_hCamera, pFrameBuffer, pThis->m_pFrameBuffer,pFrameHead);

	//�ֱ��ʸı��ˣ���ˢ�±���
	if (pThis->m_sFrInfo.iWidth != pFrameHead->iWidth || pThis->m_sFrInfo.iHeight != pFrameHead->iHeight)
	{
		pThis->m_sFrInfo.iWidth = pFrameHead->iWidth;
		pThis->m_sFrInfo.iHeight = pFrameHead->iHeight;
		pThis->InvalidateRect(NULL);//�л��ֱ��ʴ�Сʱ������������
	}
	
	if(status == CAMERA_STATUS_SUCCESS && !pThis->m_bPause)
    {
    	//����SDK��װ�õ���ʾ�ӿ�����ʾͼ��,��Ҳ���Խ�m_pFrameBuffer�е�RGB����ͨ��������ʽ��ʾ������directX,OpengGL,�ȷ�ʽ��
		CameraImageOverlay(pThis->m_hCamera, pThis->m_pFrameBuffer,pFrameHead);
        CameraDisplayRGB24(pThis->m_hCamera, pThis->m_pFrameBuffer, pFrameHead);//��������滻���û��Լ�����ʾ����
        pThis->m_iDispFrameNum++;

		pThis->ProcessSnapRequest(pThis->m_pFrameBuffer, pFrameHead);
    }    
    
	memcpy(&pThis->m_sFrInfo,pFrameHead,sizeof(tSdkFrameHead));
	
}

#else 
/*ͼ��ץȡ�̣߳���������SDK�ӿں�����ȡͼ��*/
UINT WINAPI uiDisplayThread(LPVOID lpParam)
{
	tSdkFrameHead 	sFrameInfo;
	CBasicDlg* 		pThis = (CBasicDlg*)lpParam;
	BYTE*			pbyBuffer;
	CameraSdkStatus status;
	

	while (!pThis->m_bExit)
    {   

		if(CameraGetImageBufferPriority(pThis->m_hCamera,&sFrameInfo,&pbyBuffer,1000,
			CAMERA_GET_IMAGE_PRIORITY_NEWEST) == CAMERA_STATUS_SUCCESS)
		{	
			//����õ�ԭʼ����ת����RGB��ʽ�����ݣ�ͬʱ����ISPģ�飬��ͼ����н��룬������������ɫУ���ȴ���
			//�ҹ�˾�󲿷��ͺŵ������ԭʼ���ݶ���Bayer��ʽ��
			status = CameraImageProcess(pThis->m_hCamera, pbyBuffer, pThis->m_pFrameBuffer,&sFrameInfo);//����ģʽ

			//�ֱ��ʸı��ˣ���ˢ�±���
			if (pThis->m_sFrInfo.iWidth != sFrameInfo.iWidth || pThis->m_sFrInfo.iHeight != sFrameInfo.iHeight)
			{
				pThis->m_sFrInfo.iWidth = sFrameInfo.iWidth;
				pThis->m_sFrInfo.iHeight = sFrameInfo.iHeight;
				pThis->InvalidateRect(NULL);
			}
			
			if(status == CAMERA_STATUS_SUCCESS)
            {
            	//����SDK��װ�õ���ʾ�ӿ�����ʾͼ��,��Ҳ���Խ�m_pFrameBuffer�е�RGB����ͨ��������ʽ��ʾ������directX,OpengGL,�ȷ�ʽ��
				CameraImageOverlay(pThis->m_hCamera, pThis->m_pFrameBuffer, &sFrameInfo);
                CameraDisplayRGB24(pThis->m_hCamera, pThis->m_pFrameBuffer, &sFrameInfo);
                pThis->m_iDispFrameNum++;

				pThis->ProcessSnapRequest(pThis->m_pFrameBuffer, &sFrameInfo);
            }    
            
			//�ڳɹ�����CameraGetImageBuffer�󣬱������CameraReleaseImageBuffer���ͷŻ�õ�buffer��
			//�����ٴε���CameraGetImageBufferʱ�����򽫱�����֪�������߳��е���CameraReleaseImageBuffer���ͷ���buffer
            CameraReleaseImageBuffer(pThis->m_hCamera,pbyBuffer);
            
			memcpy(&pThis->m_sFrInfo,&sFrameInfo,sizeof(tSdkFrameHead));
		}
		
    }
	
	_endthreadex(0);
    return 0;
}
#endif

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	CString	m_sAboutInfo;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	DECLARE_MESSAGE_MAP()
};


CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	m_sAboutInfo = _T("This example shows how to integrate the camera into your system in a very fast and easy way!");
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	DDX_Text(pDX, IDC_EDIT_ABOUT, m_sAboutInfo);
	//}}AFX_DATA_MAP

}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBasicDlg dialog

CBasicDlg::CBasicDlg(CWnd* pParent /*=NULL*/)
: CDialog(gLanguage == 0?IDD_BASIC_DIALOG:IDD_BASIC_DIALOG_CN, pParent)
, m_HoughParam1(180)
, m_HoughParam2(100)
, m_minRadius(0)
, m_maxRadius(30)
, m_HoughDP(2)
, m_HoughMinDist(30)
, m_GaussianBlurSizeWidth(3)
, m_GaussianBlurSizeHeight(3)
, m_Ratio(0)
, m_distance(0)
{
	//{{AFX_DATA_INIT(CBasicDlg)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_bPause = TRUE;
	m_hCamera = -1;
	m_iDispFrameNum = 0;
	m_bExit = FALSE;
	m_hDispThread = NULL;
	m_pFrameBuffer = NULL;
	m_SnapRequest = 0;
	//  m_GaussianBlurSizeWidth = 0;
	m_GaussianBlurSizeWidth = 0;
	m_GaussianBlurSizeHeight = 0;
}

void CBasicDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBasicDlg)
	DDX_Control(pDX, IDC_STATIC_PREVIEW, m_cPreview);
	//}}AFX_DATA_MAP
	DDX_Text(pDX, IDC_HOUGHPARAM1, m_HoughParam1);
	DDV_MinMaxInt(pDX, m_HoughParam1, 0, 255);
	DDX_Text(pDX, IDC_HOUGHPARAM2, m_HoughParam2);
	DDV_MinMaxInt(pDX, m_HoughParam2, 0, 255);
	DDX_Text(pDX, IDC_MINRADIUS, m_minRadius);
	DDV_MinMaxInt(pDX, m_minRadius, 0, 200);
	DDX_Text(pDX, IDC_MAXRADIUS, m_maxRadius);
	DDV_MinMaxInt(pDX, m_maxRadius, 0, 1000);
	DDX_Text(pDX, IDC_HOUGHDP, m_HoughDP);
	DDV_MinMaxInt(pDX, m_HoughDP, 1, 10);
	DDX_Text(pDX, IDC_HOUGHMINDIST, m_HoughMinDist);
	DDV_MinMaxInt(pDX, m_HoughMinDist, 10, 500);
	//  DDX_Text(pDX, IDC_sGaussianBlurSizeWidth, m_GaussianBlurSizeWidth);
	//  DDV_MinMaxInt(pDX, m_GaussianBlurSizeWidth, 1, 11);
	//  DDX_Text(pDX, IDC_sGaussianBlurSizeHeight, m_GaussianBlurSizeHeight);
	//  DDV_MinMaxInt(pDX, m_GaussianBlurSizeHeight, 0, 17);
	DDX_Text(pDX, IDC_eGaussianBlurSizeW, m_GaussianBlurSizeWidth);
	DDV_MinMaxInt(pDX, m_GaussianBlurSizeWidth, 0, 11);
	DDX_Text(pDX, IDC_eGaussianBlurSizeH, m_GaussianBlurSizeHeight);
	DDV_MinMaxInt(pDX, m_GaussianBlurSizeHeight, 0, 11);
	DDX_Text(pDX, IDC_ERATIO, m_Ratio);
	DDV_MinMaxInt(pDX, m_Ratio, 1, 5);
	DDX_Text(pDX, IDC_EDISTANCE, m_distance);
	DDV_MinMaxInt(pDX, m_distance, 300, 1000);
}

BEGIN_MESSAGE_MAP(CBasicDlg, CDialog)
	//{{AFX_MSG_MAP(CBasicDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_PREVIEW, OnButtonPreview)
	ON_BN_CLICKED(IDC_BUTTON_CAMERA_SETTINGS, OnButtonCameraSettings)
	ON_BN_CLICKED(IDC_BUTTON_SNAPSHOT, OnButtonSnapshot)
	ON_BN_CLICKED(IDC_BUTTON_SNAPSHOT_FAST, OnButtonSnapshotFast)
	ON_BN_CLICKED(IDC_BUTTON_ABOUT, OnButtonAbout)
	ON_WM_CLOSE()
	ON_WM_TIMER()
	//}}AFX_MSG_MAP

	//ON_STN_CLICKED(IDC_STATIC_INFORMATION, &CBasicDlg::OnStnClickedStaticInformation)
	//ON_BN_CLICKED(IDC_BUTTON1, &CBasicDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_HOUGHCOMMIT, &CBasicDlg::OnBnClickedHoughcommit)


	ON_BN_CLICKED(IDC_BHoughParamDark, &CBasicDlg::OnBnClickedBhoughparamdark)
	ON_BN_CLICKED(IDC_bHoughParamBright, &CBasicDlg::OnBnClickedbhoughparambright)
	ON_BN_CLICKED(IDC_CRADIOCOMMIT, &CBasicDlg::OnBnClickedCradiocommit)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBasicDlg message handlers

BOOL CBasicDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_DlgLog.Create(IDD_DIALOG_LOG,this);//����һ����Ϣ��������ʾ��־

	do 
	{
		m_DlgLog.ShowWindow(SW_SHOW);
		
// 		m_DlgLog.AppendLog("Basic Demo start");
// 		m_DlgLog.AppendLog("LoadSdkApi was called to load SDK api from MVCAMSDK.dll");
// 		m_DlgLog.AppendLog("LoadSdkApi is open source in CameraApiLoad.h ");
// 		m_DlgLog.AppendLog("It shows how to load the api from MVCAMSDK.dll,");
// 		m_DlgLog.AppendLog("you can also use your own way to load MVCAMSDK.dll");


		//Init the SDK��0:Ӣ�İ� 1:���İ� �������������������Ϣ��SDK���ɵ��豸���ý�����
		SDK_TRACE(CameraSdkInit(gLanguage),gLanguage?"��ʼ��SDK":"Init SDK");

		if (!InitCamera())
		{
			break;
		}
		
		SetTimer(0,1000,NULL);//ʹ��һ����ʱ��������֡��

		return TRUE;

	} while(0);
	
	//û���ҵ�������߳�ʼ��ʧ�ܣ��˳�����
	EndDialog(0);
	return FALSE;  // return TRUE  unless you set the focus to a control
}

void CBasicDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CBasicDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CBasicDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CBasicDlg::OnButtonPreview() 
{
	
	m_bPause = !m_bPause;
	if (gLanguage == 0)
	{
		GetDlgItem(IDC_BUTTON_PREVIEW)->SetWindowText(m_bPause?"Play":"Pause");
	}
	else
	{
		GetDlgItem(IDC_BUTTON_PREVIEW)->SetWindowText(m_bPause?"����":"��ͣ");
	}
	
	if (m_bPause)
	{
		//Set the camera in pause mode
		SDK_TRACE(CameraPause(m_hCamera),gLanguage?"��ͣ�������":"Pause the camera");
	}
	else
	{
		//Set the camera in play mode
		SDK_TRACE(CameraPlay(m_hCamera),gLanguage?"��ʼԤ��":"start preview");
	}
}


void CBasicDlg::OnButtonCameraSettings() 
{
	
	//Show the settings window��
	SDK_TRACE(CameraShowSettingPage(m_hCamera,TRUE),gLanguage?"��ʾ������������ô��ڣ��ô�����SDK�ڲ�����":"show the camera config page");
}

void CBasicDlg::OnButtonSnapshot() //ץ��һ��ͼƬ
{
	
	tSdkFrameHead FrameInfo;
	BYTE *pRawBuffer;
	BYTE *pRgbBuffer;
	CString sFileName;
	tSdkImageResolution sImageSize;
	CameraSdkStatus status;
	CString msg;
	//���ʱ��
	clock_t start, finish;
	double totaltime;

	char* str = ".bmp";
	std::string sFilePath;
	Mat midImage,matImage;

	memset(&sImageSize,0,sizeof(tSdkImageResolution));
	sImageSize.iIndex = 0xff;
	//CameraSetResolutionForSnap����ץ��ʱ�ķֱ��ʣ����Ժ�Ԥ��ʱ��ͬ��Ҳ���Բ�ͬ��
	//sImageSize.iIndex = 0xff; sImageSize.iWidth �� sImageSize.iHeight ��0����ʾץ��ʱ�ֱ��ʺ�Ԥ��ʱ��ͬ��
	//�����ϣ��ץ��ʱΪ�����ķֱ��ʣ���ο����ǵ�Snapshot���̡����߲���SDK�ֲ����й�CameraSetResolutionForSnap�ӿڵ���ϸ˵��
	SDK_TRACE(CameraSetResolutionForSnap(m_hCamera,&sImageSize),gLanguage?"����ץ��ģʽ�µķֱ���":"Set resolution for snapshot");

	//	CameraSnapToBufferץ��һ֡ͼ�����ݵ��������У��û�������SDK�ڲ�����,�ɹ����ú���Ҫ
	if((status = CameraSnapToBuffer(m_hCamera,&FrameInfo,&pRawBuffer,1000)) != CAMERA_STATUS_SUCCESS)
	{
		m_DlgLog.AppendLog("Function:[CameraSnapToBuffer] failed!");
		msg.Format("Error Code:%d. Get more information about the error in CameraDefine.h",status);
		m_DlgLog.AppendLog(msg);
		m_DlgLog.AppendLog(" ");
		MessageBox("Snapshot failed,is camera in pause mode?");
		return;
	}
	else
	{
		msg.Format("Function:[%s] SUCCESS!","CameraSnapToBuffer");
		m_DlgLog.AppendLog(msg);
		msg.Format("Description:%s.","Capture a image to the buffer in snapshot mode");
		m_DlgLog.AppendLog(msg);
		m_DlgLog.AppendLog(" ");
		
		//�ɹ�ץ�ĺ󣬱��浽�ļ�
		CString msg;
		char sCurpath[MAX_PATH];
		CString strTime = CTime::GetCurrentTime().Format(_T("%Y%m%d%H%M%S"));
		GetCurrentDirectory(MAX_PATH,sCurpath);
		sFileName.Format("%s\\Snapshot%s",sCurpath ,strTime);//��ʼ�������ļ����ļ���

		//����һ��buffer����������õ�ԭʼ����ת��ΪRGB���ݣ���ͬʱ���ͼ����Ч��
		pRgbBuffer = (BYTE *)CameraAlignMalloc(FrameInfo.iWidth*FrameInfo.iHeight*4,16);
		//Process the raw data,and get the return image in RGB format
		SDK_TRACE(CameraImageProcess(m_hCamera,pRawBuffer,pRgbBuffer,&FrameInfo),
			gLanguage?"����ͼ�񣬲��õ�RGB��ʽ������":"process the raw data to rgb data");
		
		 //Release the buffer which get from CameraSnapToBuffer or CameraGetImageBuffer
		SDK_TRACE(CameraReleaseImageBuffer(m_hCamera,pRawBuffer),
			gLanguage?"�ͷ���CameraSnapToBuffer��CameraGetImageBuffer��õ�ͼ�񻺳���":"Release the buffer malloced by CameraSnapToBuffer or CameraGetImageBuffer");
													  
		//CameraSaveImage ����ͼ�����������ʾ��α���BMPͼ�������Ҫ�����������ʽ�ģ������JPG,PNG,RAW�ȣ�
		//��ο����ǵ�Snapshot���̡����߲���SDK�ֲ����й�CameraSaveImage�ӿڵ���ϸ˵��
		if((status = CameraSaveImage(m_hCamera,sFileName.GetBuffer(1),pRgbBuffer,&FrameInfo,FILE_BMP,100)) != CAMERA_STATUS_SUCCESS)
		{
			m_DlgLog.AppendLog("Function:[CameraSaveImage] failed!");
			msg.Format("Error Code:%d. Get more information about the error in CameraDefine.h",status);
			m_DlgLog.AppendLog(msg);
			m_DlgLog.AppendLog(" ");
			msg.Format("Failed to save image to [%s] ,please check the path",sFileName);
			MessageBox(msg);
		}
		else
		{
			msg.Format("Function:[%s] SUCCESS!","CameraSaveImage");
			m_DlgLog.AppendLog(msg);
			msg.Format("Description:%s.","Save the image data in the buffer to the file");
			m_DlgLog.AppendLog(msg);
			m_DlgLog.AppendLog(" ");
			/*
			msg.Format("Snapshot one image to file:[%s.BMP]",sFileName);
			MessageBox(msg);
			*/
			/*
			cv::Mat matImage(
					cvSize(FrameInfo.iWidth,FrameInfo.iHeight), 
					FrameInfo.uiMediaType == CAMERA_MEDIA_TYPE_MONO8 ? CV_8UC1 : CV_8UC3,
					pRgbBuffer
					);
					*/
			sFileName = sFileName + str;
			//std::string sFilePath = (CString)sFileName ; 
			sFilePath = (CString)sFileName ;
			matImage = imread(sFilePath);
			m_DlgLog.AppendLog(sFileName);
			m_DlgLog.AppendLog("read file succeed!");
			
			start = clock();
				cvtColor(matImage, midImage, CV_BGR2GRAY);////ת����Ե�����ͼΪ�Ҷ�ͼ
				HoughParamDefaut();
				GaussianBlur(midImage, midImage, Size(m_GaussianBlurSizeWidth, m_GaussianBlurSizeHeight), 2, 2);//��˹ģ��ƽ��  
				vector<Vec3f> circles;//vector�洢Բ������Ͱ뾶
				//HoughCircles(midImage, circles, CV_HOUGH_GRADIENT, 8, midImage.rows / 30, 160, 80, 0, 30);//����任  				
				HoughCircles(midImage, circles, CV_HOUGH_GRADIENT, m_HoughDP, midImage.rows /m_HoughMinDist, m_HoughParam1, m_HoughParam2,m_minRadius,m_maxRadius);//����任  
				for (size_t i = 0; i < circles.size(); i++)
					{
						Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
						float radius = cvRound(circles[i][2]);
						//int radius = cvRound(circles[i][2]);
						////����Բ��
						//circle(matImage, center, 5, Scalar(0, 255, 0), -1, 8, 0);
						////����Բ����
						
						circle(matImage, center, radius, Scalar(155, 50, 255), 2, 8, 0);
						//msg.Format("�뾶��[%d],Բ������X: [%d], Բ������Y :[%d] ",cvRound(circles[i][2]),cvRound(circles[i][0]), cvRound(circles[i][1]));
						//Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
						//float radius = cvRound(circles[i][2]);
						//����Բ��
						circle(matImage, center, 2, Scalar(0, 255, 0), -1, 8, 0);
						//����Բ����
						circle(matImage, center, radius, Scalar(155, 50, 255), 2, 8, 0);
						
						msg.Format("�뾶��[%4.2f,%4.2f],Բ������X: [%4.2f,%4.2f], Բ������Y :[%4.2f,%4.2f] ",(float)cvRound(circles[i][2]),(float)cvRound(circles[i][2])/m_Ratio,(float)cvRound(circles[i][0]),(float)cvRound(circles[i][0])/m_Ratio,(float)cvRound(circles[i][1]),(float)cvRound(circles[i][1])/m_Ratio);
						//msg.Format("�뾶��[%f],Բ������X: [%6f], Բ������Y :[%6f,%6f] ",cvRound(circles[i][2]),cvRound(circles[i][0]),(float)cvRound(circles[i][0])/2.08, cvRound(circles[i][1]),(float)cvRound(circles[i][1])/2.08);
						//msg.Format("�뾶��[%d],Բ������X: [%d], Բ������Y :[%d] ",cvRound(circles[i][2]),cvRound(circles[i][0]),cvRound(circles[i][1]));
						m_DlgLog.AppendLog(msg);		
						
					}
				cv::namedWindow("���ͼ",WINDOW_NORMAL);
				cvResizeWindow("���ͼ",600,800);
				cv::imshow("���ͼ",matImage);
				
				sFileName = sFileName + str;
				sFilePath = (CString)sFileName ;
				imwrite(sFilePath, matImage);
				
				circles.swap(circles);	
				m_DlgLog.AppendLog("output circle info succeed!");
		}

		finish = clock();
		totaltime = (double)(finish - start) / CLOCKS_PER_SEC;
		msg.Format("ͼ����ʱ�䣺%f��",totaltime);
		m_DlgLog.AppendLog(msg);
		CameraAlignFree(pRgbBuffer);
	}
}

void CBasicDlg::OnButtonSnapshotFast() //ץ��һ��ͼƬ
{
	InterlockedIncrement(&m_SnapRequest);
}

void CBasicDlg::ProcessSnapRequest(BYTE* pImageData, tSdkFrameHead *pImageHead)
{
	if (m_SnapRequest == 0)
		return;
	InterlockedDecrement(&m_SnapRequest);

	static ULONG s_Index = 0;
	CString strFileName;
	GetCurrentDirectory(MAX_PATH, strFileName.GetBufferSetLength(MAX_PATH));
	strFileName.ReleaseBuffer();
	strFileName.AppendFormat("\\Snapshot%u", s_Index++);

	CameraSaveImage(m_hCamera, strFileName.GetBuffer(), pImageData, pImageHead, FILE_BMP, 100);
}

void CBasicDlg::OnButtonAbout() 
{
	CAboutDlg dlg;
	dlg.DoModal();
}

BOOL CBasicDlg::InitCamera()
{
	tSdkCameraDevInfo sCameraList[10];
	INT iCameraNums;
	CameraSdkStatus status;
	CRect rect;
	tSdkCameraCapbility sCameraInfo;
	
	//ö���豸������豸�б�
	//Enumerate camera
	iCameraNums = 10;//����CameraEnumerateDeviceǰ��������iCameraNums = 10����ʾ���ֻ��ȡ10���豸�������Ҫö�ٸ�����豸�������sCameraList����Ĵ�С��iCameraNums��ֵ
	
	if (CameraEnumerateDevice(sCameraList,&iCameraNums) != CAMERA_STATUS_SUCCESS || iCameraNums == 0)
	{
		MessageBox("No camera was found!");
		return FALSE;
	}

	//��ʾ���У�����ֻ����������һ���������ˣ�ֻ��ʼ����һ�������(-1,-1)��ʾ�����ϴ��˳�ǰ����Ĳ���������ǵ�һ��ʹ�ø�����������Ĭ�ϲ���.
	//In this demo ,we just init the first camera.
	if ((status = CameraInit(&sCameraList[0],-1,-1,&m_hCamera)) != CAMERA_STATUS_SUCCESS)
	{
		CString msg;
		msg.Format("Failed to init the camera! Error code is %d",status);
		MessageBox(msg + "��ԭ����" + CameraGetErrorString(status));
		return FALSE;
	}
	

	//Get properties description for this camera.
	SDK_TRACE(CameraGetCapability(m_hCamera,&sCameraInfo),gLanguage?"��ø��������������":"Get the capability of the camera");

	m_pFrameBuffer = (BYTE *)CameraAlignMalloc(sCameraInfo.sResolutionRange.iWidthMax*sCameraInfo.sResolutionRange.iHeightMax*4,16);	

	ASSERT(m_pFrameBuffer);
	
	//ʹ��SDK��װ�õ���ʾ�ӿ�
	//Use Mindvision SDK to display camera images.
	SDK_TRACE(CameraDisplayInit(m_hCamera,m_cPreview.GetSafeHwnd()),gLanguage?"��ʼ����ʾ�ӿ�":"Init display module");
	m_cPreview.GetClientRect(&rect);
	//Set display window size
	SDK_TRACE(CameraSetDisplaySize(m_hCamera,rect.right - rect.left,rect.bottom - rect.top),gLanguage?"������ʾ���ڴ�С":"Set display size");
	
	//֪ͨSDK�ڲ��������������ҳ�档��������Ϊ����������ơ���Ҳ���Ը���SDK�Ľӿ����Լ�ʵ���������������ҳ�棬
	//�������ǽ�����ʹ��SDK�ڲ��Զ������ķ�ʽ����ʡȥ���ڽ��濪���ϵĴ���ʱ�䡣
	//Create the settings window for the camera
	SDK_TRACE(CameraCreateSettingPage(m_hCamera,GetSafeHwnd(),
								sCameraList[0].acFriendlyName,CameraSettingPageCallback,(PVOID)this,0)
								,gLanguage?"֪ͨSDK�ڲ��������������ҳ��":"Create camera config page");
 
	#ifdef USE_CALLBACK_GRAB_IMAGE //���Ҫʹ�ûص�������ʽ������USE_CALLBACK_GRAB_IMAGE�����
	//Set the callback for image capture
	SDK_TRACE(CameraSetCallbackFunction(m_hCamera,GrabImageCallback,(PVOID)this,NULL),gLanguage?"����ͼ��ץȡ�Ļص�����":"Set image grab call back function");
	#else
	m_hDispThread = (HANDLE)_beginthreadex(NULL, 0, &uiDisplayThread, this, 0,  &m_threadID);
	ASSERT (m_hDispThread); 
	SetThreadPriority(m_hDispThread,THREAD_PRIORITY_HIGHEST);
	#endif
	//Tell the camera begin to sendding image
	SDK_TRACE(CameraPlay(m_hCamera),gLanguage?"��ʼԤ��":"Start preview");
	m_bPause = FALSE;
	GetDlgItem(IDC_BUTTON_PREVIEW)->SetWindowText(gLanguage?"��ͣ":"Pause");
	return TRUE;
}


void CBasicDlg::OnClose() 
{
	//����ʼ�����
	if (m_hCamera > 0)
	{
		if (NULL != m_hDispThread)
		{
			//�ȴ��ɼ��߳̽���
			m_bExit = TRUE;
			::WaitForSingleObject(m_hDispThread, INFINITE);
			CloseHandle(m_hDispThread);
			m_hDispThread = NULL;
		}

		//����ʼ�������
		CameraUnInit(m_hCamera);
		m_hCamera = 0;
	}

    if (m_pFrameBuffer)
    {
        CameraAlignFree(m_pFrameBuffer);
        m_pFrameBuffer = NULL;
    }
	//�����opencvͼ��
	CDialog::OnClose();
}

void CBasicDlg::OnTimer(UINT_PTR nIDEvent)//һ����ˢ��һ��ͼ����Ϣ:�ֱ��ʣ���ʾ֡�ʣ��ɼ�֡��
{
	CString strStatusText;
    int iTimeCurrnet = 0;
	static int iDispNum = 0;

   	//��SDK�ڲ��������Ĳɼ���֡������֡���ȵȡ�
    CameraGetFrameStatistic(m_hCamera, &m_sFrameCount);
    iTimeCurrnet = GetTickCount();

    if (0 != iTimeCurrnet-m_iTimeLast)
    {
        m_fCapFps = (float)((m_sFrameCount.iCapture - m_sFrameLast.iCapture)*1000.0)/(float)(iTimeCurrnet-m_iTimeLast);
        m_fDispFps = (float)((m_iDispFrameNum - iDispNum)*1000.0)/(float)(iTimeCurrnet-m_iTimeLast);
    }
    else
    {
		return;
    }        
	
    m_iTimeLast = iTimeCurrnet;

    //Update frame information
	if (gLanguage != 0)//chinese
	{
		strStatusText.Format("| ͼ��ֱ���:%d*%d | ��ʾ֡��:%4.2f FPS | ����֡��:%4.2f FPS |",
			m_sFrInfo.iWidth, m_sFrInfo.iHeight,
        m_fDispFps, m_fCapFps);
	}
	else//english
	{
		strStatusText.Format("| Resolution:%d*%d | Display rate:%4.2f FPS | Capture rate:%4.2f FPS |",
			m_sFrInfo.iWidth, m_sFrInfo.iHeight,
        m_fDispFps, m_fCapFps);
	}
	GetDlgItem(IDC_STATIC_INFORMATION)->SetWindowText(strStatusText);

    m_sFrameLast.iCapture = m_sFrameCount.iCapture;
    m_sFrameLast.iLost = m_sFrameCount.iLost;
    m_sFrameLast.iTotal = m_sFrameCount.iTotal;
    iDispNum = m_iDispFrameNum;
	
    CDialog::OnTimer(nIDEvent);
}




void CBasicDlg::OnBnClickedHoughcommit()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	//if()
	//m_HoughParam1 = 250;
	//// HoughParam2
	//m_HoughParam2 = 100;
	//// m_minRadius
	//m_minRadius = 0;
	//// m_maxRadius
	//m_maxRadius = 40;
	UpdateData(TRUE);   
	CString msg;
	msg.Format("����ֵ��GaussianBlurKsize:[%d,%d],HoughDp:[%d],minDist:[%d], Param1: [%d],Param2:[%d],MinRadius:[%d],MaxRadius:[%d] ",m_GaussianBlurSizeWidth,m_GaussianBlurSizeHeight,m_HoughDP,m_HoughMinDist,m_HoughParam1,m_HoughParam2,m_minRadius,m_maxRadius);
						
	m_DlgLog.AppendLog(msg);
	HoughParamDefaut();
	/*if(m_HoughParam1 <=0 ||m_HoughParam1>255)	m_HoughParam1 = 250;
	if(m_HoughParam2 <=0 ||m_HoughParam2>255)	m_HoughParam2 = 100;
	if(m_minRadius <=0 ||m_minRadius>=100)	m_minRadius = 0;
	if(m_maxRadius <=0 ||m_maxRadius>=1000)	m_maxRadius = 40;
	msg.Format("������Χʱ�ָ�Ĭ��ֵParam1: [%d],Param2 :[%d],MinRadius:[%d],MaxRadius:[%d] ",m_HoughParam1,m_HoughParam2,m_minRadius,m_maxRadius);
						m_DlgLog.AppendLog(msg);*/
	UpdateData(FALSE);
		
}

void CBasicDlg::HoughParamDefaut(){
	if(m_HoughDP <=0 ||m_HoughDP>11)	m_HoughDP = 2;
	
	if(m_HoughMinDist <=0 ||m_HoughMinDist>2000)	m_HoughMinDist = 30;
	if(m_HoughParam1 <=0 ||m_HoughParam1>255)	m_HoughParam1 = 170;
	if(m_HoughParam2 <=0 ||m_HoughParam2>255)	m_HoughParam2 = 35;
	if(m_minRadius <0 ||m_minRadius>=100)	m_minRadius = 0;
	if(m_maxRadius <0 ||m_maxRadius>=1000)	m_maxRadius = 30;
	if(m_GaussianBlurSizeWidth <0 ||m_GaussianBlurSizeWidth>=20)	m_GaussianBlurSizeWidth = 3;
	if(m_GaussianBlurSizeHeight <0 ||m_GaussianBlurSizeHeight>=20)	m_GaussianBlurSizeHeight = 3;
	if(m_Ratio<=0 || m_Ratio>=5) m_Ratio=1;
	CString msg;
	msg.Format("������Χʱ�ָ�Ĭ��ֵ:GaussianBlurKsize:[%d,%d],HoughDp:[%d],minDist:[%d], Param1: [%d],Param2:[%d],MinRadius:[%d],MaxRadius:[%d] ",m_GaussianBlurSizeWidth,m_GaussianBlurSizeHeight,m_HoughDP,m_HoughMinDist,m_HoughParam1,m_HoughParam2,m_minRadius,m_maxRadius);
	m_DlgLog.AppendLog(msg);

}
void CBasicDlg::HoughParamBright()
{
	CString msg;
	m_HoughDP = 2;
	m_HoughMinDist = 30;
	m_HoughParam1 = 170;
	// HoughParam2
	m_HoughParam2 = 35;
	// m_minRadius
	m_minRadius = 0;
	// m_maxRadius
	m_maxRadius = 30;
	m_GaussianBlurSizeWidth =3;
	m_GaussianBlurSizeHeight = 3;
	msg.Format("������1���óɹ���GaussianBlurKsize:[%d,%d],HoughDp:[%d],minDist:[%d], Param1: [%d],Param2:[%d],MinRadius:[%d],MaxRadius:[%d] ",m_GaussianBlurSizeWidth,m_GaussianBlurSizeHeight,m_HoughDP,m_HoughMinDist,m_HoughParam1,m_HoughParam2,m_minRadius,m_maxRadius);
	m_DlgLog.AppendLog(msg);
	UpdateData(FALSE);
}
void CBasicDlg::HoughParamDark()
{
	CString msg;
	m_HoughDP = 2;
	m_HoughMinDist = 30;
	m_HoughParam1 = 140;
	// HoughParam2
	m_HoughParam2 = 100;
	// m_minRadius
	m_minRadius = 10;
	// m_maxRadius
	m_maxRadius = 100;
	m_GaussianBlurSizeWidth =9;
	m_GaussianBlurSizeHeight = 9;
	msg.Format("������2���óɹ���GaussianBlurKsize:[%d,%d],HoughDp:[%d],minDist:[%d], Param1: [%d],Param2:[%d],MinRadius:[%d],MaxRadius:[%d] ",m_GaussianBlurSizeWidth,m_GaussianBlurSizeHeight,m_HoughDP,m_HoughMinDist,m_HoughParam1,m_HoughParam2,m_minRadius,m_maxRadius);
	m_DlgLog.AppendLog(msg);
	UpdateData(FALSE);
}




void CBasicDlg::OnBnClickedBhoughparamdark()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	HoughParamDark();
}


void CBasicDlg::OnBnClickedbhoughparambright()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	HoughParamBright();
}


void CBasicDlg::OnBnClickedCradiocommit()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	UpdateData(FALSE);
}
