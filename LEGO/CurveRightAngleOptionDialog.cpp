#include "CurveRightAngleOptionDialog.h"

CurveRightAngleOptionDialog::CurveRightAngleOptionDialog(QWidget *parent) : QDialog(parent) {
	ui.setupUi(this);

	ui.spinBoxEpsilon->setValue(16);
	ui.doubleSpinBoxCurveThreshold->setValue(2.0);
	ui.doubleSpinBoxCurveThreshold->setSingleStep(0.1);
	ui.doubleSpinBoxAngleThreshold->setValue(10.0);
	ui.doubleSpinBoxLayeringThreshold->setValue(0.7);
	ui.doubleSpinBoxLayeringThreshold->setSingleStep(0.1);
	ui.doubleSpinBoxLayeringThreshold->setMinimum(0.0);
	ui.doubleSpinBoxLayeringThreshold->setMaximum(1.0);

	ui.doubleSpinBoxSnappingThreshold->setValue(1.0);
	ui.lineEditOrientation->setText("0");

	connect(ui.pushButtonOK, SIGNAL(clicked()), this, SLOT(onOK()));
	connect(ui.pushButtonCancel, SIGNAL(clicked()), this, SLOT(onCancel()));
}

CurveRightAngleOptionDialog::~CurveRightAngleOptionDialog() {
}

int CurveRightAngleOptionDialog::getEpsilon() {
	return ui.spinBoxEpsilon->value();
}

double CurveRightAngleOptionDialog::getCurveThreshold() {
	return ui.doubleSpinBoxCurveThreshold->value();
}

double CurveRightAngleOptionDialog::getAngleThreshold() {
	return ui.doubleSpinBoxAngleThreshold->value();
}

double CurveRightAngleOptionDialog::getLayeringThreshold() {
	return ui.doubleSpinBoxLayeringThreshold->value();
}

double CurveRightAngleOptionDialog::getSnappingThreshold() {
	return ui.doubleSpinBoxSnappingThreshold->value();
}

double CurveRightAngleOptionDialog::getOrientation() {
	return ui.lineEditOrientation->text().toDouble();
}

void CurveRightAngleOptionDialog::onOK() {
	accept();
}

void CurveRightAngleOptionDialog::onCancel() {
	reject();
}