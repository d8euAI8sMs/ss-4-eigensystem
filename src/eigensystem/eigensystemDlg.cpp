
// eigensystemDlg.cpp : implementation file
//

#include "stdafx.h"
#include "eigensystem.h"
#include "eigensystemDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define WM_INVOKE WM_USER + 1234


// CEigensystemDlg dialog



UINT SimulationThreadProc(LPVOID pParam)
{
    CEigensystemDlg & dlg = * (CEigensystemDlg *) pParam;

    dlg.m_bWorking = FALSE;

    return 0;
}

CEigensystemDlg::CEigensystemDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CEigensystemDlg::IDD, pParent)
    , m_pWorkerThread(NULL)
    , m_bWorking(FALSE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CEigensystemDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CEigensystemDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_MESSAGE(WM_INVOKE, &CEigensystemDlg::OnInvoke)
    ON_BN_CLICKED(IDC_BUTTON1, &CEigensystemDlg::OnBnClickedButton1)
    ON_BN_CLICKED(IDC_BUTTON2, &CEigensystemDlg::OnBnClickedButton2)
END_MESSAGE_MAP()


// CEigensystemDlg message handlers

BOOL CEigensystemDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CEigensystemDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

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
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CEigensystemDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CEigensystemDlg::StartSimulationThread()
{
    if (this->m_bWorking)
    {
        return;
    }
    this->m_bWorking = TRUE;
    this->m_pWorkerThread = AfxBeginThread(&SimulationThreadProc, this, 0, 0, CREATE_SUSPENDED);
    this->m_pWorkerThread->m_bAutoDelete = FALSE;
    ResumeThread(this->m_pWorkerThread->m_hThread);
}


void CEigensystemDlg::StopSimulationThread()
{
    if (this->m_bWorking)
    {
        this->m_bWorking = FALSE;
        while (MsgWaitForMultipleObjects(
            1, &this->m_pWorkerThread->m_hThread, FALSE, INFINITE, QS_SENDMESSAGE) != WAIT_OBJECT_0)
        {
            MSG msg;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        //this->m_pWorkerThread->Delete();
        delete this->m_pWorkerThread;
        this->m_pWorkerThread = NULL;
    }
}


void CEigensystemDlg::Invoke(const std::function < void () > & fn)
{
    SendMessage(WM_INVOKE, 0, (LPARAM)&fn);
}


afx_msg LRESULT CEigensystemDlg::OnInvoke(WPARAM wParam, LPARAM lParam)
{
    (*(const std::function < void () > *) lParam)();
    return 0;
}

BOOL CEigensystemDlg::DestroyWindow()
{
    StopSimulationThread();

    return CDialogEx::DestroyWindow();
}


void CEigensystemDlg::OnBnClickedButton1()
{
    StartSimulationThread();
}


void CEigensystemDlg::OnBnClickedButton2()
{
    StopSimulationThread();
}
