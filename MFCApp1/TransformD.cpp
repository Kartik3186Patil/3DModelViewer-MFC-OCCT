// TransformD.cpp : implementation file
//

#include "pch.h"
#include "MFCApp1.h"
#include "afxdialogex.h"
#include "TransformD.h"
#include "Backend.h"


// TransformD dialog

IMPLEMENT_DYNAMIC(TransformD, CDialog)

TransformD::TransformD(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DIALOG1, pParent)
	, m_operationType(_T(""))
{

}

TransformD::~TransformD()
{
}

void TransformD::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_CBString(pDX, IDC_COMBO1, m_operationType);
	DDX_Control(pDX, IDC_EditDX, m_inputX);
	DDX_Control(pDX, IDC_EditDY, m_inputY);
	DDX_Control(pDX, IDC_EditDZ, m_inputZ);
	DDX_Control(pDX, IDC_COMBO1, m_inputOperation);
    m_inputOperation.SetCurSel(0);
}


BEGIN_MESSAGE_MAP(TransformD, CDialog)
	
	ON_BN_CLICKED(IDC_BUTTON1, &TransformD::OnBnClickedButton1)
    ON_CBN_SELCHANGE(IDC_COMBO1, &TransformD::OnCbnSelchangeCombo1)
END_MESSAGE_MAP()


// TransformD message handlers

void TransformD::OnBnClickedButton1()
{
    // Retrieve text from the controls
    m_inputX.GetWindowTextW(valueX);
    m_inputY.GetWindowTextW(valueY);
    m_inputZ.GetWindowTextW(valueZ);
    m_inputOperation.GetWindowTextW(valueChoice);

    // Set default values to "0" if fields are empty
    if (valueX.IsEmpty())
        valueX = _T("0");

    if (valueY.IsEmpty())
        valueY = _T("0");

    if (valueZ.IsEmpty())
        valueZ = _T("0");

   

    EndDialog(IDC_BUTTON1); // Close dialog and trigger IDOK/IDC_BUTTON1
}




void TransformD::OnCbnSelchangeCombo1()
{
    // TODO: Add your control notification handler code here
}
