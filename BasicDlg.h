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
/*��������е��������SDK�ӿ���־��Ϣ*/
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
	msg.Format(gLanguage?"����:[%s] ����ʧ��!":"Function:[%s] return error",FuncName);\
	m_DlgLog.AppendLog(msg);\
	msg.Format(gLanguage?"������:%d. ��ο�CameraStatus.h�д��������ϸ����":"Error code:%d.refer to CameraStatus.h for more information",status);\
	m_DlgLog.AppendLog(msg);\
	}\
	else\
	{\
	msg.Format(gLanguage?"����:[%s] ���óɹ�!":"Function:[%s] success",FuncName);\
	m_DlgLog.AppendLog(msg);\
	msg.Format(gLanguage?"����:%s.":"Action:%s",TXT);\
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
	CStatic	        m_cPreview;//������ʾ���ͼ��Ĵ��ڿؼ�|the control used to display the images.
	CameraHandle    m_hCamera;	//������豸���|the handle of the camera we use
	tSdkFrameHead   m_sFrInfo;//���ڱ��浱ǰͼ��֡��֡ͷ��Ϣ

	int	            m_iDispFrameNum;//���ڼ�¼��ǰ�Ѿ���ʾ��ͼ��֡������
	float           m_fDispFps;//��ʾ֡��
	float           m_fCapFps;//����֡��
    tSdkFrameStatistic  m_sFrameCount;
    tSdkFrameStatistic  m_sFrameLast;
    int		        m_iTimeLast;

	BYTE*           m_pFrameBuffer;//���ڽ�ԭʼͼ������ת��ΪRGB�Ļ�����
	BOOL	        m_bPause;//�Ƿ���ͣͼ��

    UINT            m_threadID;//ͼ��ץȡ�̵߳�ID
	HANDLE          m_hDispThread;//ͼ��ץȡ�̵߳ľ��
	BOOL            m_bExit;//����֪ͨͼ��ץȡ�߳̽���
	LONG			m_SnapRequest;//��ͼ����

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
	imageΪ����ͼ��Ҫ���ǻҶ�ͼ��
	circlesΪ���Բ������ÿ�������������������͵�Ԫ�أ�Բ�ĺ����꣬Բ���������Բ�뾶��
	methodΪʹ�û���任Բ�����㷨��Opencv2.4.13�Ĳ�����CV_HOUGH_GRADIENT��
	dpΪ��һ�׶���ʹ�õĻ���ռ�ķֱ��ʣ�dp=1ʱ��ʾ����ռ�������ͼ��ռ�Ĵ�Сһ�£�dp=2ʱ����ռ�������ͼ��ռ��һ�룬�Դ����ƣ�
	minDistΪԲ��֮�����С���룬�����⵽������Բ��֮�����С�ڸ�ֵ������Ϊ������ͬһ��Բ�ģ�
	param1Ϊ��Ե���ʱʹ��Canny���ӵĸ���ֵ��
	param2Ϊ����1.5�Ͳ���2.5�������е���ֵ��
	minRadius��maxRadiusΪ����⵽��Բ�뾶����Сֵ�����ֵ��Ĭ��ֵ0��
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
	// dpΪ��һ�׶���ʹ�õĻ���ռ�ķֱ��ʣ�dp=1ʱ��ʾ����ռ�������ͼ��ռ�Ĵ�Сһ�£�dp=2ʱ����ռ�������ͼ��ռ��һ�룬�Դ�����
	int m_HoughDP;
	// minDistΪԲ��֮�����С���룬�����⵽������Բ��֮�����С�ڸ�ֵ������Ϊ������ͬһ��Բ��
	int m_HoughMinDist;
	// ��˹�ں˵Ĵ�С������ksize.width��ksize.height���Բ�ͬ�������Ƕ�����Ϊ���������������ߣ����ǿ�������ģ����Ƕ�����sigma���������
//	int m_GaussianBlurSizeWidth;
	// ��˹�ں˵Ĵ�С������ksize.width��ksize.height���Բ�ͬ�������Ƕ�����Ϊ��������������������⣩�����ߣ����ǿ�������ģ����Ƕ�����sigma���������
//	int m_GaussianBlurSizeHeight;
	int m_GaussianBlurSizeWidth;
	int m_GaussianBlurSizeHeight;
	afx_msg void OnBnClickedBhoughparamdark();
	afx_msg void OnBnClickedbhoughparambright();

	// ����תʵ�ʳ��ȵı�����
	float m_Ratio;

	// ��λ��mm�����ƣ����200�����1000
	int m_distance;

	afx_msg void OnBnClickedCradiocommit();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BASICDLG_H__DE07E1D0_D0B7_4FA5_A4F3_45499366E00E__INCLUDED_)
