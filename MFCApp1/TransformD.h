#pragma once
#include "afxdialogex.h"


// TransformD dialog

class TransformD : public CDialog
{
	DECLARE_DYNAMIC(TransformD)

public:
	TransformD(CWnd* pParent = nullptr);   // standard constructor
	virtual ~TransformD();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG1 };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	
	afx_msg void OnBnClickedButton1();
	CString m_operationType;
	CEdit m_inputX;
	CEdit m_inputY;
	CEdit m_inputZ;
	CComboBox m_inputOperation;
	CString valueX;
	CString valueY;
	CString valueZ;
	CString valueChoice;


	afx_msg void OnCbnSelchangeCombo1();
};
