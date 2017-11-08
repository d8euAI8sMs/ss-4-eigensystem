
// eigensystemDlg.cpp : implementation file
//

#include "stdafx.h"
#include "eigensystem.h"
#include "eigensystemDlg.h"
#include "afxdialogex.h"

#include "model.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define WM_INVOKE WM_USER + 1234


// CEigensystemDlg dialog

using namespace plot;
using namespace util;
using namespace math;
using namespace model;

using points_t = std::vector < point < double > > ;
using plot_t   = simple_list_plot < points_t > ;

const size_t n_points    = 1000;
const size_t n_wavefuncs = 6;

plot_t phase_plot, wavefunc_re_plots[n_wavefuncs];
world_t::ptr_t phase_fixed_bound = world_t::create(), wavefunc_fixed_bound = world_t::create();

UINT SimulationThreadProc(LPVOID pParam)
{
    CEigensystemDlg & dlg = * (CEigensystemDlg *) pParam;

    int l;
    double e_start, e_end, b_w, b_h, m_i;

    // get modeling properties

    dlg.Invoke([&] () {
        l = dlg.m_nOrbitalMomentum;
        e_start = dlg.m_fpStartEnergy;
        e_end = dlg.m_fpEndEnergy;
        b_w = dlg.m_fpBarrierWidth;
        b_h = dlg.m_fpBarrierHeight;
        m_i = dlg.m_lfModelingInterval;
        for (size_t k = 0; k < n_wavefuncs; ++k)
        {
            dlg.m_cVisibilityChecks[k].ShowWindow(SW_HIDE);
            dlg.m_cEnergyLevels[k].ShowWindow(SW_HIDE);
        }
    });

    auto barrier_fn = make_barrier_fn(b_w, b_h);

    // update fixed bounds

    phase_fixed_bound->xmin = e_start;
    phase_fixed_bound->xmax = e_end;

    // calculate phase plot

    phase_plot.data->clear();
    for (size_t i = 0; (i < n_points) && dlg.m_bWorking; ++i)
    {
        double e = e_start + (double) i / (n_points) * (e_end - e_start);
        auto wavefunc_dfunc = make_wavefunc_dfunc(barrier_fn, b_w, e, l);
        auto result = rk4_solve3ia < rv3 > (wavefunc_dfunc,
                                            0,
                                            b_w * m_i,
                                            b_w / n_points,
                                            rv3 { 0, M_PI / 2 },
                                            1e-8,
                                            0.01);
        phase_plot.data->emplace_back(e, result.x.at<1>());
        if ((i % (n_points / 100)) == 0)
        {
            phase_plot.refresh();
            dlg.m_cEigenvaluePlot.RedrawBuffer();
            dlg.m_cEigenvaluePlot.SwapBuffers();
            dlg.Invoke([&] () {
                dlg.m_cEigenvaluePlot.RedrawWindow();
            });
        }
    }

    phase_plot.refresh();
    dlg.m_cEigenvaluePlot.RedrawBuffer();
    dlg.m_cEigenvaluePlot.SwapBuffers();
    dlg.Invoke([&] () {
        dlg.m_cEigenvaluePlot.RedrawWindow();
    });

    if (!dlg.m_bWorking)
    {
        return 0;
    }

    wavefunc_re_plots[0].refresh();

    // find up to `n_wavefunc` eigenvalues

    double energy_levels[n_wavefuncs] = {};
    size_t level_count = min(n_wavefuncs, max(abs(phase_plot.data->back().y - phase_plot.data->front().y) / M_PI + 0.5, 0));
    size_t min_level = (size_t) std::floor(std::abs(phase_plot.data->front().y / M_PI - 0.5));

    size_t left = 0;
    for (size_t i = 0; i < level_count; ++i)
    {
        double thr = - (2 * ((int) i + (int) min_level) + 1) * M_PI / 2;
        for (size_t j = left; j < n_points - 1; ++j)
        {
            if ((phase_plot.data->at(j).y >= thr) && (phase_plot.data->at(j + 1).y <= thr))
            {
                size_t iters = 0;
                double left_e = phase_plot.data->at(j).x;
                double right_e = phase_plot.data->at(j + 1).x;
                double left_phi = phase_plot.data->at(j).y;
                double right_phi = phase_plot.data->at(j + 1).y;
                double cur_e;
                double phi;
                do
                {
                    cur_e = (left_e + right_e) / 2;
                    auto wavefunc_dfunc = make_wavefunc_dfunc(barrier_fn, b_w, cur_e, l);
                    auto result = rk4_solve3ia < rv3 > (wavefunc_dfunc,
                                                        0,
                                                        b_w * m_i,
                                                        b_w / n_points,
                                                        rv3 { 0, M_PI / 2 },
                                                        1e-8,
                                                        0.01);
                    phi = result.x.at<1>();
                    if ((left_phi >= thr) && (phi <= thr))
                    {
                        right_phi = thr;
                        right_e   = cur_e;
                    }
                    else
                    {
                        left_phi = thr;
                        left_e   = cur_e;
                    }
                    ++iters;
                } while ((iters < 1000) && ((right_e - left_e) > 1e-8));
                energy_levels[i] = (left_e + right_e) / 2;
                left = j;
                break;
            }
        }
    }

    dlg.Invoke([&] () {
        CString str;
        for (size_t k = 0; k < level_count; ++k)
        {
            str.Format(_T("E%u"), k + min_level);
            dlg.m_cVisibilityChecks[k].SetWindowText(str);
            dlg.m_cVisibilityChecks[k].ShowWindow(SW_SHOW);
            str.Format(_T("E%u=%.5lf"), k + min_level, energy_levels[k]);
            dlg.m_cEnergyLevels[k].SetWindowText(str);
            dlg.m_cEnergyLevels[k].ShowWindow(SW_SHOW);
        }
    });

    // draw `level_count` wave functions

    for (size_t i = 0; i < n_wavefuncs; ++i)
    {
        wavefunc_re_plots[i].data->clear();
    }

    wavefunc_re_plots[0].auto_world->clear();

    for (size_t i = 0; (i < level_count) && dlg.m_bWorking; ++i)
    {
        auto wavefunc_dfunc = make_wavefunc_dfunc(barrier_fn, b_w, energy_levels[i], l);
        double s_step = b_w / (n_points / 10);
        dresult3 < rv3 > result = { 0, { 0, M_PI / 2 } };
        for (size_t j = 0; (result.t < b_w * m_i) && dlg.m_bWorking; ++j)
        {
            result = rk4_solve3a < rv3 > (wavefunc_dfunc, result.t, s_step, result.x, 1e-8, 0.01);
            if (!wavefunc_re_plots[i].data->empty() &&
                (result.t - wavefunc_re_plots[i].data->back().x < s_step / 10))
            {
                continue;
            }
            auto wavefunc_value = make_wavefunc_value(b_w, result.t, result.x.at<0>(), result.x.at<1>());
            wavefunc_re_plots[i].data->emplace_back(result.t, wavefunc_value);
            wavefunc_re_plots[i].data->insert(wavefunc_re_plots[i].data->begin(), point<double>(-result.t, wavefunc_value));
            if (((wavefunc_re_plots[i].data->size() / 2) % (n_points / 100)) == 0)
            {
                wavefunc_re_plots[0].auto_world->adjust(*wavefunc_re_plots[i].data);
                wavefunc_re_plots[0].auto_world->flush();
                dlg.m_cEigenfunctionPlot.RedrawBuffer();
                dlg.m_cEigenfunctionPlot.SwapBuffers();
                dlg.Invoke([&] () {
                    dlg.m_cEigenfunctionPlot.RedrawWindow();
                    for (size_t k = 0; k < n_wavefuncs; ++k)
                    {
                        wavefunc_re_plots[k].view->visible = dlg.m_cVisibilityChecks[k].GetCheck();
                    }
                    double w = b_w * min(1.5, m_i);
                    if (dlg.m_cDrawAtFullInterval.GetCheck())
                    {
                        w = b_w * m_i;
                    }
                    wavefunc_fixed_bound->xmin = -w;
                    wavefunc_fixed_bound->xmax = w;
                });
            }
        }
    }

    // exit

    dlg.m_bWorking = FALSE;

    return 0;
}

CEigensystemDlg::CEigensystemDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CEigensystemDlg::IDD, pParent)
    , m_pWorkerThread(NULL)
    , m_bWorking(FALSE)
    , m_fpBarrierWidth(3)
    , m_fpBarrierHeight(2)
    , m_nOrbitalMomentum(0)
    , m_fpStartEnergy(-2)
    , m_fpEndEnergy(0)
    , m_lfModelingInterval(2)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CEigensystemDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_EDIT1, m_fpBarrierWidth);
    DDX_Text(pDX, IDC_EDIT2, m_fpBarrierHeight);
    DDX_Text(pDX, IDC_EDIT3, m_nOrbitalMomentum);
    DDX_Text(pDX, IDC_EDIT4, m_fpStartEnergy);
    DDX_Text(pDX, IDC_EDIT5, m_fpEndEnergy);
    DDX_Control(pDX, IDC_CHECK1, m_cVisibilityChecks[0]);
    DDX_Control(pDX, IDC_CHECK2, m_cVisibilityChecks[1]);
    DDX_Control(pDX, IDC_CHECK3, m_cVisibilityChecks[2]);
    DDX_Control(pDX, IDC_CHECK4, m_cVisibilityChecks[3]);
    DDX_Control(pDX, IDC_CHECK5, m_cVisibilityChecks[4]);
    DDX_Control(pDX, IDC_CHECK6, m_cVisibilityChecks[5]);
    DDX_Control(pDX, IDC_CHECK7, m_cVisibilityChecks[6]);
    DDX_Control(pDX, IDC_EL0, m_cEnergyLevels[0]);
    DDX_Control(pDX, IDC_EL1, m_cEnergyLevels[1]);
    DDX_Control(pDX, IDC_EL2, m_cEnergyLevels[2]);
    DDX_Control(pDX, IDC_EL3, m_cEnergyLevels[3]);
    DDX_Control(pDX, IDC_EL4, m_cEnergyLevels[4]);
    DDX_Control(pDX, IDC_EL5, m_cEnergyLevels[5]);
    DDX_Control(pDX, IDC_CHECK8, m_cDrawAtFullInterval);
    DDX_Control(pDX, IDC_EV_PLOT, m_cEigenvaluePlot);
    DDX_Control(pDX, IDC_EF_PLOT, m_cEigenfunctionPlot);
    DDX_Text(pDX, IDC_EDIT6, m_lfModelingInterval);
}

BEGIN_MESSAGE_MAP(CEigensystemDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_MESSAGE(WM_INVOKE, &CEigensystemDlg::OnInvoke)
    ON_BN_CLICKED(IDC_BUTTON1, &CEigensystemDlg::OnBnClickedButton1)
    ON_BN_CLICKED(IDC_BUTTON2, &CEigensystemDlg::OnBnClickedButton2)
    ON_COMMAND_RANGE(IDC_CHECK1, IDC_CHECK8, &CEigensystemDlg::OnVisibilityCheckChanged)
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

    m_cVisibilityChecks[6].SetCheck(FALSE);

    for (size_t k = 0; k < n_wavefuncs; ++k)
    {
        m_cVisibilityChecks[k].SetCheck(TRUE);
        m_cVisibilityChecks[k].ShowWindow(SW_HIDE);
        m_cEnergyLevels[k].ShowWindow(SW_HIDE);
    }

    auto_viewport_params params;
    params.factors = { 0, 0, 0.1, 0.1 };
    params.fixed   = { true, true, false, false };
    auto phase_avp    = min_max_auto_viewport < points_t > :: create();
    auto wavefunc_avp = min_max_auto_viewport < points_t > :: create();
    params.fixed_bound = phase_fixed_bound;
    phase_avp->set_params(params);
    params.upper = { false, false, true, true };
    params.upper_bound = world_t::create(0, 0, -1.1, 1.1);
    params.fixed_bound = wavefunc_fixed_bound;
    wavefunc_avp->set_params(params);

    phase_plot
        .with_view()
        .with_view_line_pen(plot::palette::pen(RGB(255, 255, 255), 2))
        .with_data()
        .with_auto_viewport(phase_avp);

    std::vector < drawable::ptr_t > wavefunc_layers;

    for (size_t i = 0; i < n_wavefuncs; ++i)
    {
        wavefunc_re_plots[i]
            .with_view()
            .with_view_line_pen(plot::palette::pen(RGB(127 + rand() % 128, 127 + rand() % 128, 255), 2))
            .with_data()
            .with_auto_viewport(wavefunc_avp);
        wavefunc_layers.push_back(wavefunc_re_plots[i].view);
    }

    m_cEigenvaluePlot.background = palette::brush();
    m_cEigenfunctionPlot.background = palette::brush();

    m_cEigenvaluePlot.triple_buffered = true;
    m_cEigenfunctionPlot.triple_buffered = true;

    custom_drawable::ptr_t phase_lines = custom_drawable::create
    (
        [] (CDC & dc, const viewport & vp)
        {
            auto pen = palette::pen(0xffffff, 2);
            dc.SelectObject(pen.get());
            size_t k_max = std::ceil(std::abs(vp.world.ymin / M_PI + 0.5));
            for (size_t i = 0; i < k_max; ++i)
            {
                dc.MoveTo(vp.world_to_screen().xy({ vp.world.xmin, - (2 * (int) i + 1) * M_PI / 2 }));
                dc.LineTo(vp.world_to_screen().xy({ vp.world.xmax, - (2 * (int) i + 1) * M_PI / 2 }));
            }
        }
    );

    custom_drawable::ptr_t barrier_lines = custom_drawable::create
    (
        [&] (CDC & dc, const viewport & vp)
        {
            auto pen = palette::pen(0xffffff, 3);
            dc.SelectObject(pen.get());
            double r;
            this->Invoke([&] () {
                r = this->m_fpBarrierWidth;
            });
            dc.MoveTo(vp.world_to_screen().xy({ -r, vp.world.ymin }));
            dc.LineTo(vp.world_to_screen().xy({ -r, vp.world.ymax }));
            dc.MoveTo(vp.world_to_screen().xy({ +r, vp.world.ymin }));
            dc.LineTo(vp.world_to_screen().xy({ +r, vp.world.ymax }));
        }
    );

    wavefunc_layers.push_back(barrier_lines);

    m_cEigenvaluePlot.plot_layer.with(
        viewporter::create(
            tick_drawable::create(
                layer_drawable::create(std::vector < drawable::ptr_t > ({
                    phase_plot.view, phase_lines
                })),
                const_n_tick_factory<axe::x>::create(
                    make_simple_tick_formatter(2, 5),
                    0,
                    5
                ),
                const_n_tick_factory<axe::y>::create(
                    make_simple_tick_formatter(2, 5),
                    0,
                    5
                ),
                palette::pen(RGB(80, 80, 80)),
                RGB(200, 200, 200)
            ),
            make_viewport_mapper(phase_plot.viewport_mapper)
        )
    );

    m_cEigenfunctionPlot.plot_layer.with(
        viewporter::create(
            tick_drawable::create(
                layer_drawable::create(wavefunc_layers),
                const_n_tick_factory<axe::x>::create(
                    make_simple_tick_formatter(2, 5),
                    0,
                    5
                ),
                const_n_tick_factory<axe::y>::create(
                    make_simple_tick_formatter(2, 5),
                    0,
                    5
                ),
                palette::pen(RGB(80, 80, 80)),
                RGB(200, 200, 200)
            ),
            make_viewport_mapper(wavefunc_re_plots[0].viewport_mapper)
        )
    );

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
    UpdateData(TRUE);

    StartSimulationThread();
}


void CEigensystemDlg::OnBnClickedButton2()
{
    StopSimulationThread();
}


afx_msg void CEigensystemDlg::OnVisibilityCheckChanged(UINT nID)
{
    if (this->m_bWorking)
    {
        return;
    }

    wavefunc_re_plots[0].auto_world->clear();

    for (size_t k = 0; k < n_wavefuncs; ++k)
    {
        wavefunc_re_plots[k].view->visible = this->m_cVisibilityChecks[k].GetCheck();
        if (wavefunc_re_plots[k].view->visible)
        {
            wavefunc_re_plots[0].auto_world->adjust(*wavefunc_re_plots[k].data);
        }
    }

    double w = this->m_fpBarrierWidth * min(1.5, this->m_lfModelingInterval);
    if (this->m_cDrawAtFullInterval.GetCheck())
    {
        w = this->m_fpBarrierWidth * this->m_lfModelingInterval;
    }
    wavefunc_fixed_bound->xmin = -w;
    wavefunc_fixed_bound->xmax = w;

    wavefunc_re_plots[0].auto_world->flush();

    this->m_cEigenfunctionPlot.RedrawBuffer();
    this->m_cEigenfunctionPlot.SwapBuffers();
    this->m_cEigenfunctionPlot.RedrawWindow();
}
