
// eigensystemDlg.h : header file
//

#pragma once

#include <functional>

#include <util/common/plot/PlotStatic.h>

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
    afx_msg void OnVisibilityCheckChanged(UINT nID);
    double m_fpBarrierWidth;
    double m_fpBarrierHeight;
    int m_nOrbitalMomentum;
    double m_fpStartEnergy;
    double m_fpEndEnergy;
    CButton m_cVisibilityChecks[7];
    CStatic m_cEnergyLevels[6];
    PlotStatic m_cEigenvaluePlot;
    PlotStatic m_cEigenfunctionPlot;
    double m_lfModelingInterval;
    CButton m_cDrawAtFullInterval;
    BOOL m_bAccurateNearZero;
};
