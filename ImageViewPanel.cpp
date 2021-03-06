#pragma GCC diagnostic ignored "-Wwrite-strings"

#include "ImageViewPanel.h"

#include "res/btn_actual.xpm"
#include "res/btn_auto.xpm"
#include "res/btn_close.xpm"
#include "res/btn_fit.xpm"
#include "res/btn_next.xpm"
#include "res/btn_pause.xpm"
#include "res/btn_prev.xpm"
#include "res/btn_random.xpm"
#include "res/btn_save.xpm"
#include "res/img_picture.xpm"
#include <algorithm>
#include <cmath>
#include <ctime>
#include <wx/mstream.h>
#include <wx/notebook.h>
#include <wx/statline.h>
#include <wx/wfstream.h>

#ifdef USE_CIMG
using cimg_library::CImg;
#endif

ImageViewPanel::ImageViewPanel(wxWindow *parent, Entry *entry, wxWindowID id,
                               const wxPoint &pos, const wxSize &size,
                               long style, const wxString &name)
    : wxPanel(parent, id, pos, size, style, name), timer(this),
      randomEngine(r()), image(IMG_PICTURE_xpm) {
  auto outerSizer = new wxBoxSizer(wxVERTICAL);
  auto btnSizer = new wxBoxSizer(wxHORIZONTAL);
  btnClose = new wxButton(this, wxID_CLOSE, "Close", wxDefaultPosition,
                          wxDefaultSize, wxBU_EXACTFIT | wxBU_NOTEXT);
  btnClose->SetBitmap(wxICON(IDI_ICON_CLOSE));
  btnClose->SetToolTip("Close this tab.");
  btnClose->Bind(wxEVT_BUTTON, &ImageViewPanel::OnCloseButtonClick, this);

  spnScale = new wxSpinCtrl(this, wxID_ANY);
  spnScale->SetToolTip("Set the percentage the image is scaled to.");
  spnScale->SetValue(100);
  spnScale->SetRange(1, 200);
  spnScale->Bind(wxEVT_SPINCTRL, &ImageViewPanel::OnScaleValueChanged, this);

  btnFitSize = new wxToggleButton(this, wxID_ANY, "Fit", wxDefaultPosition,
                                  wxDefaultSize, wxBU_EXACTFIT | wxBU_NOTEXT);
  btnFitSize->SetBitmap(wxICON(IDI_ICON_FIT));
  btnFitSize->SetToolTip("Fit the image inside the tab.");

  btnFitSize->Bind(wxEVT_TOGGLEBUTTON, &ImageViewPanel::OnBtnFitSizeToggle,
                   this);
  btnFitSize->SetValue(true);

  btnActualSize = new wxButton(this, wxID_ANY, "Actual", wxDefaultPosition,
                               wxDefaultSize, wxBU_EXACTFIT | wxBU_NOTEXT);
  btnActualSize->Bind(wxEVT_BUTTON, &ImageViewPanel::OnBtnActualSizePressed,
                      this);
  btnActualSize->SetBitmap(wxICON(IDI_ICON_ACTUAL));
  btnActualSize->SetToolTip("Scale the image back to the original size.");
  btnActualSize->Disable();
  spnScale->Disable();
  btnSizer->Add(btnClose, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL);
  btnSizer->AddSpacer(5);
  btnSizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition,
                                 wxDefaultSize, wxLI_VERTICAL),
                0, wxRIGHT | wxALIGN_CENTER_VERTICAL | wxEXPAND, 5);
  btnSizer->Add(spnScale, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 5);
  btnSizer->Add(new wxStaticText(this, wxID_ANY, "%", wxDefaultPosition,
                                 wxDefaultSize, wxST_NO_AUTORESIZE),
                wxRIGHT | wxALIGN_CENTER_VERTICAL, 5);
  btnSizer->Add(btnFitSize, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 5);
  btnSizer->Add(btnActualSize, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 5);
  btnSizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition,
                                 wxDefaultSize, wxLI_VERTICAL),
                0, wxRIGHT | wxALIGN_CENTER_VERTICAL | wxEXPAND, 5);

  btnNext = new wxButton(this, wxID_ANY, "Next", wxDefaultPosition,
                         wxDefaultSize, wxBU_EXACTFIT | wxBU_NOTEXT);
  btnNext->Bind(wxEVT_BUTTON, [this](wxCommandEvent &event) { Advance(1); },
                wxID_ANY);
  btnNext->SetBitmap(wxICON(IDI_ICON_NEXT));
  btnNext->SetToolTip("Load the next image from the same directory.");
  btnPrev = new wxButton(this, wxID_ANY, "Prev", wxDefaultPosition,
                         wxDefaultSize, wxBU_EXACTFIT | wxBU_NOTEXT);
  btnPrev->Bind(wxEVT_BUTTON, &ImageViewPanel::OnPrevButtonClick, this);
  btnPrev->SetBitmap(wxICON(IDI_ICON_PREV));
  btnPrev->SetToolTip("Load the previous image from the same directory.");

  btnAuto = new wxToggleButton(this, wxID_ANY, "Auto", wxDefaultPosition,
                               wxDefaultSize, wxBU_EXACTFIT | wxBU_NOTEXT);
  btnAuto->Bind(wxEVT_TOGGLEBUTTON, &ImageViewPanel::OnBtnAutoToggle, this);
  btnAuto->SetBitmap(wxICON(IDI_ICON_AUTO));
  btnAuto->SetBitmapPressed(wxICON(IDI_ICON_PAUSE));
  btnAuto->SetToolTip("Auto advance to the next image");

  btnRandom = new wxToggleButton(this, wxID_ANY, "Auto", wxDefaultPosition,
                                 wxDefaultSize, wxBU_EXACTFIT | wxBU_NOTEXT);
  btnRandom->Bind(wxEVT_TOGGLEBUTTON, &ImageViewPanel::OnBtnAutoToggle, this);
  btnRandom->SetBitmap(wxICON(IDI_ICON_RANDOM));
  btnRandom->SetToolTip("Advance image randomly");

  spnRefreshTime = new wxSpinCtrl(this, wxID_ANY);
  spnRefreshTime->SetValue(30);
  spnRefreshTime->SetRange(1, 3000);
  spnRefreshTime->SetToolTip("Set the time interval (seconds) between the "
                             "current image to the next one.");

  btnSizer->Add(btnPrev, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 5);
  btnSizer->Add(btnNext, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 5);
  btnSizer->Add(btnAuto, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 5);
  btnSizer->Add(spnRefreshTime, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL);
  btnSizer->Add(new wxStaticText(this, wxID_ANY, "s", wxDefaultPosition,
                                 wxDefaultSize, wxST_NO_AUTORESIZE),
                0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 5);
  btnSizer->Add(btnRandom, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 5);

  auto btnSave = new wxButton(this, wxID_ANY, "Save As", wxDefaultPosition,
                              wxDefaultSize, wxBU_EXACTFIT | wxBU_NOTEXT);
  btnSave->SetBitmap(wxICON(IDI_ICON_SAVE));
  btnSave->SetToolTip("Save file as");
  btnSave->Bind(wxEVT_BUTTON, &ImageViewPanel::OnSaveButtonClick, this);

  btnSizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition,
                                 wxDefaultSize, wxLI_VERTICAL),
                0, wxRIGHT | wxALIGN_CENTER_VERTICAL | wxEXPAND, 5);
  btnSizer->Add(btnSave, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 5);

  outerSizer->Add(btnSizer, 0, wxALL, 10);
  outerSizer->Add(new wxStaticLine(this), 0,
                  wxRIGHT | wxALIGN_CENTER_VERTICAL | wxEXPAND, 10);
  scrollPanel = new wxScrolledWindow(this, wxID_ANY);
  scrollSizer = new wxBoxSizer(wxVERTICAL);

  scrollPanel->SetSizerAndFit(scrollSizer);
  scrollPanel->SetScrollRate(10, 10);

  outerSizer->Add(scrollPanel, 1, wxEXPAND | wxALL, 10);
  SetSizer(outerSizer);

  auto entryParent = entry->Parent();
  for (int i = 0; i < entryParent->Count(); ++i) {
    Entry *childEntry = (*entryParent)[i];

    if (childEntry->IsDirectory())
      continue;
    auto ext = childEntry->Name().AfterLast('.').Lower();

    if (ext != "jpg" && ext != "jpeg" && ext != "png" && ext != "gif")
      continue;

    entries.push_back(childEntry);
  }

  entryIter = std::find(entries.begin(), entries.end(), entry);

  bitmapStatic = new wxStaticBitmap(scrollPanel, wxID_ANY, wxBitmap(image));
  scrollSizer->Add(bitmapStatic);

  Bind(wxEVT_TIMER, &ImageViewPanel::OnTimerNotify, this);

  if (entries.size() > 0)
    random = std::uniform_int_distribution<int>(1, entries.size());
  else
    random = std::uniform_int_distribution<int>(0, 1);

  #ifdef USE_CIMG
    cimgImage = CImgFromWxImage(image);
  #endif
}

void ImageViewPanel::OnCloseButtonClick(wxCommandEvent &event) {
  auto notebook = dynamic_cast<wxNotebook *>(GetParent());
  if (!notebook)
    return;
  auto currentPage = notebook->FindPage(this);
  if (currentPage == wxNOT_FOUND)
    return;
  notebook->DeletePage(currentPage);
}

void ImageViewPanel::OnScaleValueChanged(wxSpinEvent &event) {
  RefreshImage(event.GetPosition());
}

void ImageViewPanel::OnBtnActualSizePressed(wxCommandEvent &event) {
  spnScale->SetValue(100);
  RefreshImage();
}

void ImageViewPanel::OnBtnFitSizeToggle(wxCommandEvent &event) {
  if (btnFitSize->GetValue()) {
    FitImage();
    btnActualSize->Disable();
    spnScale->Disable();
  } else {
    btnActualSize->Enable();
    spnScale->Enable();
  }
}

void ImageViewPanel::FitImage() {
  auto ctrlSize = scrollPanel->GetSize();
  auto imageSize = image.GetSize();

  auto widthPercent = (ctrlSize.GetWidth() * 100) / imageSize.GetWidth();
  auto heightPercent = (ctrlSize.GetHeight() * 100) / imageSize.GetHeight();

  auto &percentage =
      widthPercent < heightPercent ? widthPercent : heightPercent;
  percentage = percentage > 200 ? 200 : percentage;
  spnScale->SetValue(percentage);
  RefreshImage(percentage);
}

void ImageViewPanel::RefreshImage(const int &percentage) {
  if (percentage == 100) {
    bitmapStatic->SetBitmap(wxBitmap(image));
  } else {
    int width = (image.GetWidth() * percentage) / 100;
    int height = (image.GetHeight() * percentage) / 100;

    bitmapStatic->SetBitmap(wxBitmap(CreateScaledImage(width, height, true)));
  }
  scrollSizer->FitInside(scrollPanel);
  spnScale->SetValue(percentage);
  Refresh();
  Update();
}

void ImageViewPanel::OnNextButtonClick(wxCommandEvent &event) { Advance(1); }

void ImageViewPanel::OnPrevButtonClick(wxCommandEvent &event) { Advance(-1); }

void ImageViewPanel::SetImageEntry(Entry *entry) {
  auto notebook = dynamic_cast<wxNotebook *>(GetParent());
  if (!notebook)
    return;
  auto currentPage = notebook->FindPage(this);
  if (currentPage == wxNOT_FOUND)
    return;
  notebook->SetPageText(currentPage, entry->Name());

  image = entry->CreateImage();
#ifdef USE_CIMG
  cimgImage = CImgFromWxImage(image);
  if (image.HasAlpha()) {
    cimgAlpha = CImgAlphaFromWxImage(image);
  }
#endif
  if (btnFitSize->GetValue())
    FitImage();
  else
    RefreshImage();
}

bool ImageViewPanel::Layout() {
  wxPanel::Layout();
  if (btnFitSize->GetValue())
    FitImage();
  return false;
}

void ImageViewPanel::Advance(const int &step) {
  if (step == 0)
    return;
  bool isForward = step >= 0;
  for (int i = 0; i < std::abs(step); i++) {
    if (isForward) {
      entryIter++;
      if (entryIter == entries.end())
        entryIter = entries.begin();
    } else {
      entryIter--;
      if (entryIter < entries.begin())
        entryIter = entries.end() - 1;
    }
  }

  SetImageEntry(*entryIter);
}

void ImageViewPanel::OnBtnAutoToggle(wxCommandEvent &event) {
  if (btnAuto->GetValue()) {
    timer.Start(spnRefreshTime->GetValue() * 1000);
    btnRandom->Disable();
    spnRefreshTime->Disable();
    btnPrev->Disable();
    btnNext->Disable();
  } else {
    timer.Stop();
    btnRandom->Enable();
    spnRefreshTime->Enable();
    btnPrev->Enable();
    btnNext->Enable();
  }
}

void ImageViewPanel::OnTimerNotify(wxTimerEvent &timer) {
  Advance(btnRandom->GetValue() ? random(randomEngine) : 1);
}

void ImageViewPanel::OnSaveButtonClick(wxCommandEvent &event) {
  Entry *entry = *entryIter;
  wxFileDialog dialog(this, "Save As...", filename.GetPath(), entry->Name(),
                      wxFileSelectorDefaultWildcardStr, wxFD_SAVE);
  if (dialog.ShowModal() != wxID_OK)
    return;

  filename = dialog.GetPath();
  wxFileOutputStream output(dialog.GetPath());
  entry->WriteStream(output);
  output.Close();
}

void ImageViewPanel::SwitchToTheActualImage() { SetImageEntry(*entryIter); }

wxImage ImageViewPanel::CreateScaledImage(const unsigned int &width,
                                          const unsigned int &height,
                                          const bool isHighQuality) {
  if (width == 0 || height == 0)
    return wxImage(1, 1);

  if (!isHighQuality)
    return image.Scale(width, height);
#ifdef USE_CIMG
  constexpr auto quality = 6;
  auto scaled =
      cimgImage.get_resize(width, height, 1, cimgImage.spectrum(), quality);

  if (image.HasAlpha()) {
    auto scaledAlpha =
        cimgAlpha.get_resize(width, height, 1, cimgAlpha.spectrum(), quality);
    return wxImageFromCImg(scaled, &scaledAlpha);
  } else {
    return wxImageFromCImg(scaled);
  }
#else
  return image.Scale(width, height, wxIMAGE_QUALITY_HIGH);
#endif
}

#ifdef USE_CIMG
/* FIXME:
Currently CImg is used for image processing. CImg expects deinterleaved image
data which is different than what wxImage expects. This results in having to
juggle arround the data buffer, dimension value, etc. to make things work.
Certainly it causes performance impacts.
CImg should be replaced with other code that process interleaved image
buffer.

This option is disabled by default due to performance problems.
*/

CImg<unsigned char> ImageViewPanel::CImgFromWxImage(const wxImage &image) {
  CImg<unsigned char> output(image.GetData(), 3, image.GetWidth(),
                             image.GetHeight(), 1);
  output.permute_axes("yzcx");
  return output;
}

CImg<unsigned char> ImageViewPanel::CImgAlphaFromWxImage(const wxImage &image) {
  if (!image.HasAlpha())
    return CImg<unsigned char>();

  return CImg<unsigned char>(image.GetAlpha(), image.GetWidth(),
                             image.GetHeight(), 1, 1);
}

wxImage ImageViewPanel::wxImageFromCImg(const CImg<unsigned char> &img,
                                        CImg<unsigned char> *alpha) {
  auto width = img.width();
  auto height = img.height();
  auto dataSize = width * height * 3;

  CImg<unsigned char> rearranged = img.get_permute_axes("cxyz");

  auto data = (unsigned char *)malloc(dataSize);
  memcpy(data, rearranged.data(), dataSize);

  wxImage output(width, height);
  output.SetData(data);

  if (alpha != nullptr) {
    auto alphaSize = width * height;

    auto alphaData = (unsigned char *)malloc(alphaSize);
    memcpy(alphaData, alpha->data(), alphaSize);

    output.SetAlpha(alphaData);
  }
  return output;
}
#endif
