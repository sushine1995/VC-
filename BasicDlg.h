// BasicDlg.h : header file
//
//BIG5 TRANS ALLOWED

#if !defined(AFX_BASICDLG_H__DE07E1D0_D0B7_4FA5_A4F3_45499366E00E__INCLUDED_)
#define AFX_BASICDLG_H__DE07E1D0_D0B7_4FA5_A4F3_45499366E00E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SdkCallTrace.h"

#include "CameraApi.h"	 
#ifdef _WIN64
#pragma comment(lib, "..\\MVCAMSDK_X64.lib")
#else
#pragma comment(lib, "..\\MVCAMSDK.lib")
#endif
/*输出例程中调用相机的SDK接口日志信息*/
#define SDK_TRACE(_FUNC_,TXT) \
{\
	CameraSdkStatus status;\
	CString msg;\
	CString FuncName;\
	FuncName = #_FUNC_;\
	FuncName = FuncName.Left(FuncName.FindOneOf("("));\
\
	status = _FUNC_;\
	if (status != CAMERA_STATUS_SUCCESS)\
	{\
	msg.Format(gLanguage?"函数:[%s] 调用失败!":"Function:[%s] return error",FuncName);\
	m_DlgLog.AppendLog(msg);\
	msg.Format(gLanguage?"错误码:%d. 请参考CameraStatus.h中错误码的详细定义":"Error code:%d.refer to CameraStatus.h for more information",status);\
	m_DlgLog.AppendLog(msg);\
	}\
	else\
	{\
	msg.Format(gLanguage?"函数:[%s] 调用成功!":"Function:[%s] success",FuncName);\
	m_DlgLog.AppendLog(msg);\
	msg.Format(gLanguage?"功能:%s.":"Action:%s",TXT);\
	m_DlgLog.AppendLog(msg);\
	}\
	msg = "";\
	m_DlgLog.AppendLog(msg);\
}


/////////////////////////////////////////////////////////////////////////////
// CBasicDlg dialog

class CBasicDlg : public CDialog
{
// Construction
public:
	CBasicDlg(CWnd* pParent = NULL);	// standard constructor
	CSdkCallTrace m_DlgLog;
// Dialog Data
	//{{AFX_DATA(CBasicDlg)
	enum { IDD = IDD_BASIC_DIALOG_CN };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBasicDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation

protected:
	HICON m_hIcon;
	
	// Generated message map functions
	//{{AFX_MSG(CBaseDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnButtonPreview();
	afx_msg void OnButtonCameraSettings();
	afx_msg void OnButtonSnapshot();
	afx_msg void OnButtonSnapshotFast();
	afx_msg void OnButtonAbout();
	afx_msg void OnClose();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
		
public:
	CStatic	        m_cPreview;//用于显示相机图像的窗口控件|the control used to display the images.
	CameraHandle    m_hCamera;	//相机的设备句柄|the handle of the camera we use
	tSdkFrameHead   m_sFrInfo;//用于保存当前图像帧的帧头信息

	int	            m_iDispFrameNum;//用于记录当前已经显示的图像帧的数量
	float           m_fDispFps;//显示帧率
	float           m_fCapFps;//捕获帧率
    tSdkFrameStatistic  m_sFrameCount;
    tSdkFrameStatistic  m_sFrameLast;
    int		        m_iTimeLast;

	BYTE*           m_pFrameBuffer;//用于将原始图像数据转换为RGB的缓冲区
	BOOL	        m_bPause;//是否暂停图像

    UINT            m_threadID;//图像抓取线程的ID
	HANDLE          m_hDispThread;//图像抓取线程的句柄
	BOOL            m_bExit;//用来通知图像抓取线程结束
	LONG			m_SnapRequest;//截图请求

	BOOL InitCamera();
	void ProcessSnapRequest(BYTE* pImageData, tSdkFrameHead *pImageHead);

	/*
	void cv::HoughCircles( InputArray _image, OutputArray _circles,
                       int method, double dp, double min_dist,
                       double param1, double param2,
                       int minRadius, int maxRadius )
{
    Ptr<CvMemStorage> storage = cvCreateMemStorage(STORAGE_SIZE);
    Mat image = _image.getMat();
    CvMat c_image = image;
    CvSeq* seq = cvHoughCircles( &c_image, storage, method,
                    dp, min_dist, param1, param2, minRadius, maxRadius );
    seqToMat(seq, _circles);
}
	image为输入图像，要求是灰度图像；
	circles为输出圆向量，每个向量包括三个浮点型的元素：圆心横坐标，圆心纵坐标和圆半径；
	method为使用霍夫变换圆检测的算法，Opencv2.4.13的参数是CV_HOUGH_GRADIENT；
	dp为第一阶段所使用的霍夫空间的分辨率，dp=1时表示霍夫空间与输入图像空间的大小一致，dp=2时霍夫空间是输入图像空间的一半，以此类推；
	minDist为圆心之间的最小距离，如果检测到的两个圆心之间距离小于该值，则认为它们是同一个圆心；
	param1为边缘检测时使用Canny算子的高阈值；
	param2为步骤1.5和步骤2.5中所共有的阈值；
	minRadius和maxRadius为所检测到的圆半径的最小值和最大值，默认值0；
	*/
	void HoughParamDefaut();
	void HoughParamDark();
	void HoughParamBright();
	// HoughParam1
	int m_HoughParam1 ;
	// HoughParam2
	int m_HoughParam2 ;
	// m_minRadius
	int m_minRadius ;
	// m_maxRadius
	int m_maxRadius ;

	afx_msg void OnBnClickedHoughcommit();
	// dp为第一阶段所使用的霍夫空间的分辨率，dp=1时表示霍夫空间与输入图像空间的大小一致，dp=2时霍夫空间是输入图像空间的一半，以此类推
	int m_HoughDP;
	// minDist为圆心之间的最小距离，如果检测到的两个圆心之间距离小于该值，则认为它们是同一个圆心
	int m_HoughMinDist;
	// 高斯内核的大小。其中ksize.width和ksize.height可以不同，但他们都必须为正数和奇数。或者，它们可以是零的，它们都是由sigma计算而来。
//	int m_GaussianBlurSizeWidth;
	// 高斯内核的大小。其中ksize.width和ksize.height可以不同，但他们都必须为正数和奇数（并不能理解）。或者，它们可以是零的，它们都是由sigma计算而来。
//	int m_GaussianBlurSizeHeight;
	int m_GaussianBlurSizeWidth;
	int m_GaussianBlurSizeHeight;
	afx_msg void OnBnClickedBhoughparamdark();
	afx_msg void OnBnClickedbhoughparambright();

	// 像素转实际长度的比例尺
	float m_Ratio;

	// 单位：mm，限制：最低200，最大1000
	int m_distance;

	afx_msg void OnBnClickedCradiocommit();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BASICDLG_H__DE07E1D0_D0B7_4FA5_A4F3_45499366E00E__INCLUDED_)
