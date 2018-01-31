#include "OurCustomOptionDialog.h"

OurCustomOptionDialog::OurCustomOptionDialog(QWidget *parent) : QDialog(parent) {
	ui.setupUi(this);

	ui.spinBoxResolution->setValue(5);
	ui.doubleSpinBoxLayeringThreshold->setValue(0.7);

	connect(ui.pushButtonOK, SIGNAL(clicked()), this, SLOT(onOK()));
	connect(ui.pushButtonCancel, SIGNAL(clicked()), this, SLOT(onCancel()));
}

OurCustomOptionDialog::~OurCustomOptionDialog() {
}

int OurCustomOptionDialog::getResolution() {
	return ui.spinBoxResolution->value();
}

double OurCustomOptionDialog::getLayeringThreshold() {
	return ui.doubleSpinBoxLayeringThreshold->value();
}

void OurCustomOptionDialog::onOK() {
	accept();
}

void OurCustomOptionDialog::onCancel() {
	reject();
}
