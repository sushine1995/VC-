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

/*相机配置窗口的消息回调函数
hCamera:当前相机的句柄
MSG:消息类型，
	SHEET_MSG_LOAD_PARAM_DEFAULT	= 0,//加载默认参数的按钮被点击，加载默认参数完成后触发该消息,
	SHEET_MSG_LOAD_PARAM_GROUP		= 1,//切换参数组完成后触发该消息,
	SHEET_MSG_LOAD_PARAM_FROMFILE	= 2,//加载参数按钮被点击，已从文件中加载相机参数后触发该消息
	SHEET_MSG_SAVE_PARAM_GROUP		= 3//保存参数按钮被点击，参数保存后触发该消息
	具体参见CameraDefine.h中emSdkPropSheetMsg类型

uParam:消息附带的参数，不同的消息，参数意义不同。
	当 MSG 为 SHEET_MSG_LOAD_PARAM_DEFAULT时，uParam表示被加载成默认参数组的索引号，从0开始，分别对应A,B,C,D四组
	当 MSG 为 SHEET_MSG_LOAD_PARAM_GROUP时，uParam表示切换后的参数组的索引号，从0开始，分别对应A,B,C,D四组
	当 MSG 为 SHEET_MSG_LOAD_PARAM_FROMFILE时，uParam表示被文件中参数覆盖的参数组的索引号，从0开始，分别对应A,B,C,D四组
	当 MSG 为 SHEET_MSG_SAVE_PARAM_GROUP时，uParam表示当前保存的参数组的索引号，从0开始，分别对应A,B,C,D四组
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
如果需要使用回调函数的方式获得图像数据，则反注释宏定义USE_CALLBACK_GRAB_IMAGE.
我们的SDK同时支持回调函数和主动调用接口抓取图像的方式。两种方式都采用了"零拷贝"机制，以最大的程度的降低系统负荷，提高程序执行效率。
但是主动抓取方式比回调函数的方式更加灵活，可以设置超时等待时间等，我们建议您使用 uiDisplayThread 中的方式
*/
//#define USE_CALLBACK_GRAB_IMAGE 

#ifdef USE_CALLBACK_GRAB_IMAGE
/*图像抓取回调函数*/
void _stdcall GrabImageCallback(CameraHandle hCamera, BYTE *pFrameBuffer, tSdkFrameHead* pFrameHead,PVOID pContext)
{
	
	CameraSdkStatus status;
	CBasicDlg *pThis = (CBasicDlg*)pContext;
	
	//将获得的原始数据转换成RGB格式的数据，同时经过ISP模块，对图像进行降噪，边沿提升，颜色校正等处理。
	//我公司大部分型号的相机，原始数据都是Bayer格式的
	status = CameraImageProcess(pThis->m_hCamera, pFrameBuffer, pThis->m_pFrameBuffer,pFrameHead);

	//分辨率改变了，则刷新背景
	if (pThis->m_sFrInfo.iWidth != pFrameHead->iWidth || pThis->m_sFrInfo.iHeight != pFrameHead->iHeight)
	{
		pThis->m_sFrInfo.iWidth = pFrameHead->iWidth;
		pThis->m_sFrInfo.iHeight = pFrameHead->iHeight;
		pThis->InvalidateRect(NULL);//切换分辨率大小时，擦除背景。
	}
	
	if(status == CAMERA_STATUS_SUCCESS && !pThis->m_bPause)
    {
    	//调用SDK封装好的显示接口来显示图像,您也可以将m_pFrameBuffer中的RGB数据通过其他方式显示，比如directX,OpengGL,等方式。
		CameraImageOverlay(pThis->m_hCamera, pThis->m_pFrameBuffer,pFrameHead);
        CameraDisplayRGB24(pThis->m_hCamera, pThis->m_pFrameBuffer, pFrameHead);//这里可以替换成用户自己的显示函数
        pThis->m_iDispFrameNum++;

		pThis->ProcessSnapRequest(pThis->m_pFrameBuffer, pFrameHead);
    }    
    
	memcpy(&pThis->m_sFrInfo,pFrameHead,sizeof(tSdkFrameHead));
	
}

#else 
/*图像抓取线程，主动调用SDK接口函数获取图像*/
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
			//将获得的原始数据转换成RGB格式的数据，同时经过ISP模块，对图像进行降噪，边沿提升，颜色校正等处理。
			//我公司大部分型号的相机，原始数据都是Bayer格式的
			status = CameraImageProcess(pThis->m_hCamera, pbyBuffer, pThis->m_pFrameBuffer,&sFrameInfo);//连续模式

			//分辨率改变了，则刷新背景
			if (pThis->m_sFrInfo.iWidth != sFrameInfo.iWidth || pThis->m_sFrInfo.iHeight != sFrameInfo.iHeight)
			{
				pThis->m_sFrInfo.iWidth = sFrameInfo.iWidth;
				pThis->m_sFrInfo.iHeight = sFrameInfo.iHeight;
				pThis->InvalidateRect(NULL);
			}
			
			if(status == CAMERA_STATUS_SUCCESS)
            {
            	//调用SDK封装好的显示接口来显示图像,您也可以将m_pFrameBuffer中的RGB数据通过其他方式显示，比如directX,OpengGL,等方式。
				CameraImageOverlay(pThis->m_hCamera, pThis->m_pFrameBuffer, &sFrameInfo);
                CameraDisplayRGB24(pThis->m_hCamera, pThis->m_pFrameBuffer, &sFrameInfo);
                pThis->m_iDispFrameNum++;

				pThis->ProcessSnapRequest(pThis->m_pFrameBuffer, &sFrameInfo);
            }    
            
			//在成功调用CameraGetImageBuffer后，必须调用CameraReleaseImageBuffer来释放获得的buffer。
			//否则再次调用CameraGetImageBuffer时，程序将被挂起，知道其他线程中调用CameraReleaseImageBuffer来释放了buffer
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

	m_DlgLog.Create(IDD_DIALOG_LOG,this);//创建一个信息窗口来显示日志

	do 
	{
		m_DlgLog.ShowWindow(SW_SHOW);
		
// 		m_DlgLog.AppendLog("Basic Demo start");
// 		m_DlgLog.AppendLog("LoadSdkApi was called to load SDK api from MVCAMSDK.dll");
// 		m_DlgLog.AppendLog("LoadSdkApi is open source in CameraApiLoad.h ");
// 		m_DlgLog.AppendLog("It shows how to load the api from MVCAMSDK.dll,");
// 		m_DlgLog.AppendLog("you can also use your own way to load MVCAMSDK.dll");


		//Init the SDK，0:英文版 1:中文版 ，作用于相机的描述信息和SDK生成的设备配置界面上
		SDK_TRACE(CameraSdkInit(gLanguage),gLanguage?"初始化SDK":"Init SDK");

		if (!InitCamera())
		{
			break;
		}
		
		SetTimer(0,1000,NULL);//使用一个定时器来计算帧率

		return TRUE;

	} while(0);
	
	//没有找到相机或者初始化失败，退出程序
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
		GetDlgItem(IDC_BUTTON_PREVIEW)->SetWindowText(m_bPause?"播放":"暂停");
	}
	
	if (m_bPause)
	{
		//Set the camera in pause mode
		SDK_TRACE(CameraPause(m_hCamera),gLanguage?"暂停相机工作":"Pause the camera");
	}
	else
	{
		//Set the camera in play mode
		SDK_TRACE(CameraPlay(m_hCamera),gLanguage?"开始预览":"start preview");
	}
}


void CBasicDlg::OnButtonCameraSettings() 
{
	
	//Show the settings window。
	SDK_TRACE(CameraShowSettingPage(m_hCamera,TRUE),gLanguage?"显示相机的属性配置窗口，该窗口由SDK内部生成":"show the camera config page");
}

void CBasicDlg::OnButtonSnapshot() //抓拍一张图片
{
	
	tSdkFrameHead FrameInfo;
	BYTE *pRawBuffer;
	BYTE *pRgbBuffer;
	CString sFileName;
	tSdkImageResolution sImageSize;
	CameraSdkStatus status;
	CString msg;
	//检测时间
	clock_t start, finish;
	double totaltime;

	char* str = ".bmp";
	std::string sFilePath;
	Mat midImage,matImage;

	memset(&sImageSize,0,sizeof(tSdkImageResolution));
	sImageSize.iIndex = 0xff;
	//CameraSetResolutionForSnap设置抓拍时的分辨率，可以和预览时相同，也可以不同。
	//sImageSize.iIndex = 0xff; sImageSize.iWidth 和 sImageSize.iHeight 置0，表示抓拍时分辨率和预览时相同。
	//如果您希望抓拍时为单独的分辨率，请参考我们的Snapshot例程。或者参阅SDK手册中有关CameraSetResolutionForSnap接口的详细说明
	SDK_TRACE(CameraSetResolutionForSnap(m_hCamera,&sImageSize),gLanguage?"设置抓拍模式下的分辨率":"Set resolution for snapshot");

	//	CameraSnapToBuffer抓拍一帧图像数据到缓冲区中，该缓冲区由SDK内部申请,成功调用后，需要
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
		
		//成功抓拍后，保存到文件
		CString msg;
		char sCurpath[MAX_PATH];
		CString strTime = CTime::GetCurrentTime().Format(_T("%Y%m%d%H%M%S"));
		GetCurrentDirectory(MAX_PATH,sCurpath);
		sFileName.Format("%s\\Snapshot%s",sCurpath ,strTime);//初始化保存文件的文件名

		//申请一个buffer，用来将获得的原始数据转换为RGB数据，并同时获得图像处理效果
		pRgbBuffer = (BYTE *)CameraAlignMalloc(FrameInfo.iWidth*FrameInfo.iHeight*4,16);
		//Process the raw data,and get the return image in RGB format
		SDK_TRACE(CameraImageProcess(m_hCamera,pRawBuffer,pRgbBuffer,&FrameInfo),
			gLanguage?"处理图像，并得到RGB格式的数据":"process the raw data to rgb data");
		
		 //Release the buffer which get from CameraSnapToBuffer or CameraGetImageBuffer
		SDK_TRACE(CameraReleaseImageBuffer(m_hCamera,pRawBuffer),
			gLanguage?"释放由CameraSnapToBuffer、CameraGetImageBuffer获得的图像缓冲区":"Release the buffer malloced by CameraSnapToBuffer or CameraGetImageBuffer");
													  
		//CameraSaveImage 保存图像，这里仅仅演示如何保存BMP图像。如果需要保存成其他格式的，里如果JPG,PNG,RAW等，
		//请参考我们的Snapshot例程。或者参阅SDK手册中有关CameraSaveImage接口的详细说明
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
				cvtColor(matImage, midImage, CV_BGR2GRAY);////转化边缘检测后的图为灰度图
				HoughParamDefaut();
				GaussianBlur(midImage, midImage, Size(m_GaussianBlurSizeWidth, m_GaussianBlurSizeHeight), 2, 2);//高斯模糊平滑  
				vector<Vec3f> circles;//vector存储圆心坐标和半径
				//HoughCircles(midImage, circles, CV_HOUGH_GRADIENT, 8, midImage.rows / 30, 160, 80, 0, 30);//霍夫变换  				
				HoughCircles(midImage, circles, CV_HOUGH_GRADIENT, m_HoughDP, midImage.rows /m_HoughMinDist, m_HoughParam1, m_HoughParam2,m_minRadius,m_maxRadius);//霍夫变换  
				for (size_t i = 0; i < circles.size(); i++)
					{
						Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
						float radius = cvRound(circles[i][2]);
						//int radius = cvRound(circles[i][2]);
						////绘制圆心
						//circle(matImage, center, 5, Scalar(0, 255, 0), -1, 8, 0);
						////绘制圆轮廓
						
						circle(matImage, center, radius, Scalar(155, 50, 255), 2, 8, 0);
						//msg.Format("半径：[%d],圆心坐标X: [%d], 圆心坐标Y :[%d] ",cvRound(circles[i][2]),cvRound(circles[i][0]), cvRound(circles[i][1]));
						//Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
						//float radius = cvRound(circles[i][2]);
						//绘制圆心
						circle(matImage, center, 2, Scalar(0, 255, 0), -1, 8, 0);
						//绘制圆轮廓
						circle(matImage, center, radius, Scalar(155, 50, 255), 2, 8, 0);
						
						msg.Format("半径：[%4.2f,%4.2f],圆心坐标X: [%4.2f,%4.2f], 圆心坐标Y :[%4.2f,%4.2f] ",(float)cvRound(circles[i][2]),(float)cvRound(circles[i][2])/m_Ratio,(float)cvRound(circles[i][0]),(float)cvRound(circles[i][0])/m_Ratio,(float)cvRound(circles[i][1]),(float)cvRound(circles[i][1])/m_Ratio);
						//msg.Format("半径：[%f],圆心坐标X: [%6f], 圆心坐标Y :[%6f,%6f] ",cvRound(circles[i][2]),cvRound(circles[i][0]),(float)cvRound(circles[i][0])/2.08, cvRound(circles[i][1]),(float)cvRound(circles[i][1])/2.08);
						//msg.Format("半径：[%d],圆心坐标X: [%d], 圆心坐标Y :[%d] ",cvRound(circles[i][2]),cvRound(circles[i][0]),cvRound(circles[i][1]));
						m_DlgLog.AppendLog(msg);		
						
					}
				cv::namedWindow("结果图",WINDOW_NORMAL);
				cvResizeWindow("结果图",600,800);
				cv::imshow("结果图",matImage);
				
				sFileName = sFileName + str;
				sFilePath = (CString)sFileName ;
				imwrite(sFilePath, matImage);
				
				circles.swap(circles);	
				m_DlgLog.AppendLog("output circle info succeed!");
		}

		finish = clock();
		totaltime = (double)(finish - start) / CLOCKS_PER_SEC;
		msg.Format("图像处理时间：%f秒",totaltime);
		m_DlgLog.AppendLog(msg);
		CameraAlignFree(pRgbBuffer);
	}
}

void CBasicDlg::OnButtonSnapshotFast() //抓拍一张图片
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
	
	//枚举设备，获得设备列表
	//Enumerate camera
	iCameraNums = 10;//调用CameraEnumerateDevice前，先设置iCameraNums = 10，表示最多只读取10个设备，如果需要枚举更多的设备，请更改sCameraList数组的大小和iCameraNums的值
	
	if (CameraEnumerateDevice(sCameraList,&iCameraNums) != CAMERA_STATUS_SUCCESS || iCameraNums == 0)
	{
		MessageBox("No camera was found!");
		return FALSE;
	}

	//该示例中，我们只假设连接了一个相机。因此，只初始化第一个相机。(-1,-1)表示加载上次退出前保存的参数，如果是第一次使用该相机，则加载默认参数.
	//In this demo ,we just init the first camera.
	if ((status = CameraInit(&sCameraList[0],-1,-1,&m_hCamera)) != CAMERA_STATUS_SUCCESS)
	{
		CString msg;
		msg.Format("Failed to init the camera! Error code is %d",status);
		MessageBox(msg + "，原因是" + CameraGetErrorString(status));
		return FALSE;
	}
	

	//Get properties description for this camera.
	SDK_TRACE(CameraGetCapability(m_hCamera,&sCameraInfo),gLanguage?"获得该相机的特性描述":"Get the capability of the camera");

	m_pFrameBuffer = (BYTE *)CameraAlignMalloc(sCameraInfo.sResolutionRange.iWidthMax*sCameraInfo.sResolutionRange.iHeightMax*4,16);	

	ASSERT(m_pFrameBuffer);
	
	//使用SDK封装好的显示接口
	//Use Mindvision SDK to display camera images.
	SDK_TRACE(CameraDisplayInit(m_hCamera,m_cPreview.GetSafeHwnd()),gLanguage?"初始化显示接口":"Init display module");
	m_cPreview.GetClientRect(&rect);
	//Set display window size
	SDK_TRACE(CameraSetDisplaySize(m_hCamera,rect.right - rect.left,rect.bottom - rect.top),gLanguage?"设置显示窗口大小":"Set display size");
	
	//通知SDK内部建该相机的属性页面。窗口名称为该相机的名称。您也可以根据SDK的接口来自己实现相机的属性配置页面，
	//但是我们建议您使用SDK内部自动创建的方式，来省去您在界面开发上的大量时间。
	//Create the settings window for the camera
	SDK_TRACE(CameraCreateSettingPage(m_hCamera,GetSafeHwnd(),
								sCameraList[0].acFriendlyName,CameraSettingPageCallback,(PVOID)this,0)
								,gLanguage?"通知SDK内部建该相机的属性页面":"Create camera config page");
 
	#ifdef USE_CALLBACK_GRAB_IMAGE //如果要使用回调函数方式，定义USE_CALLBACK_GRAB_IMAGE这个宏
	//Set the callback for image capture
	SDK_TRACE(CameraSetCallbackFunction(m_hCamera,GrabImageCallback,(PVOID)this,NULL),gLanguage?"设置图像抓取的回调函数":"Set image grab call back function");
	#else
	m_hDispThread = (HANDLE)_beginthreadex(NULL, 0, &uiDisplayThread, this, 0,  &m_threadID);
	ASSERT (m_hDispThread); 
	SetThreadPriority(m_hDispThread,THREAD_PRIORITY_HIGHEST);
	#endif
	//Tell the camera begin to sendding image
	SDK_TRACE(CameraPlay(m_hCamera),gLanguage?"开始预览":"Start preview");
	m_bPause = FALSE;
	GetDlgItem(IDC_BUTTON_PREVIEW)->SetWindowText(gLanguage?"暂停":"Pause");
	return TRUE;
}


void CBasicDlg::OnClose() 
{
	//反初始化相机
	if (m_hCamera > 0)
	{
		if (NULL != m_hDispThread)
		{
			//等待采集线程结束
			m_bExit = TRUE;
			::WaitForSingleObject(m_hDispThread, INFINITE);
			CloseHandle(m_hDispThread);
			m_hDispThread = NULL;
		}

		//反初始化相机。
		CameraUnInit(m_hCamera);
		m_hCamera = 0;
	}

    if (m_pFrameBuffer)
    {
        CameraAlignFree(m_pFrameBuffer);
        m_pFrameBuffer = NULL;
    }
	//如果有opencv图像
	CDialog::OnClose();
}

void CBasicDlg::OnTimer(UINT_PTR nIDEvent)//一秒中刷新一次图像信息:分辨率，显示帧率，采集帧率
{
	CString strStatusText;
    int iTimeCurrnet = 0;
	static int iDispNum = 0;

   	//从SDK内部获得相机的采集总帧数，丢帧数等等。
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
		strStatusText.Format("| 图像分辨率:%d*%d | 显示帧率:%4.2f FPS | 捕获帧率:%4.2f FPS |",
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
	// TODO: 在此添加控件通知处理程序代码
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
	msg.Format("输入值：GaussianBlurKsize:[%d,%d],HoughDp:[%d],minDist:[%d], Param1: [%d],Param2:[%d],MinRadius:[%d],MaxRadius:[%d] ",m_GaussianBlurSizeWidth,m_GaussianBlurSizeHeight,m_HoughDP,m_HoughMinDist,m_HoughParam1,m_HoughParam2,m_minRadius,m_maxRadius);
						
	m_DlgLog.AppendLog(msg);
	HoughParamDefaut();
	/*if(m_HoughParam1 <=0 ||m_HoughParam1>255)	m_HoughParam1 = 250;
	if(m_HoughParam2 <=0 ||m_HoughParam2>255)	m_HoughParam2 = 100;
	if(m_minRadius <=0 ||m_minRadius>=100)	m_minRadius = 0;
	if(m_maxRadius <=0 ||m_maxRadius>=1000)	m_maxRadius = 40;
	msg.Format("超出范围时恢复默认值Param1: [%d],Param2 :[%d],MinRadius:[%d],MaxRadius:[%d] ",m_HoughParam1,m_HoughParam2,m_minRadius,m_maxRadius);
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
	msg.Format("超出范围时恢复默认值:GaussianBlurKsize:[%d,%d],HoughDp:[%d],minDist:[%d], Param1: [%d],Param2:[%d],MinRadius:[%d],MaxRadius:[%d] ",m_GaussianBlurSizeWidth,m_GaussianBlurSizeHeight,m_HoughDP,m_HoughMinDist,m_HoughParam1,m_HoughParam2,m_minRadius,m_maxRadius);
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
	msg.Format("参数组1设置成功：GaussianBlurKsize:[%d,%d],HoughDp:[%d],minDist:[%d], Param1: [%d],Param2:[%d],MinRadius:[%d],MaxRadius:[%d] ",m_GaussianBlurSizeWidth,m_GaussianBlurSizeHeight,m_HoughDP,m_HoughMinDist,m_HoughParam1,m_HoughParam2,m_minRadius,m_maxRadius);
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
	msg.Format("参数组2设置成功：GaussianBlurKsize:[%d,%d],HoughDp:[%d],minDist:[%d], Param1: [%d],Param2:[%d],MinRadius:[%d],MaxRadius:[%d] ",m_GaussianBlurSizeWidth,m_GaussianBlurSizeHeight,m_HoughDP,m_HoughMinDist,m_HoughParam1,m_HoughParam2,m_minRadius,m_maxRadius);
	m_DlgLog.AppendLog(msg);
	UpdateData(FALSE);
}




void CBasicDlg::OnBnClickedBhoughparamdark()
{
	// TODO: 在此添加控件通知处理程序代码
	HoughParamDark();
}


void CBasicDlg::OnBnClickedbhoughparambright()
{
	// TODO: 在此添加控件通知处理程序代码
	HoughParamBright();
}


void CBasicDlg::OnBnClickedCradiocommit()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(FALSE);
}
