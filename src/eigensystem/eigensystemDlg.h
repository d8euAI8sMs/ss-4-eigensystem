
// eigensystemDlg.h : header file
//

#pragma once

#include <functional>

// CEigensystemDlg dialog
class CEigensystemDlg : public CDialogEx
{
// Construction
public:
	CEigensystemDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_EIGENSYSTEM_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;
    CWinThread * m_pWorkerThread;
    volatile BOOL m_bWorking;
    void Invoke(const std::function < void () > & fn);
    afx_msg LRESULT OnInvoke(WPARAM wParam, LPARAM lParam);
    void StartSimulationThread();
    void StopSimulationThread();

    friend UINT SimulationThreadProc(LPVOID pParam);

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
    virtual BOOL DestroyWindow();
    afx_msg void OnBnClickedButton1();
    afx_msg void OnBnClickedButton2();
};
