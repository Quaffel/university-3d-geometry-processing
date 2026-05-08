#include "MeshSmoothingToolbar.hh"
//#include <iostream>

MeshSmoothingToolbar::MeshSmoothingToolbar(QWidget * parent)
    : QWidget(parent)
{
    setupUi(this);

    // For these changes, we can keep the iteration history
    connect(sb_iters,             &QSpinBox::valueChanged,       this, &MeshSmoothingToolbar::minorSettingsChanged);
    connect(dsb_feat_alpha,       &QDoubleSpinBox::valueChanged, this, &MeshSmoothingToolbar::minorSettingsChanged);
    connect(cb_enhance,           &QCheckBox::toggled,           this, &MeshSmoothingToolbar::minorSettingsChanged);
    connect(cb_volume_preserve,   &QCheckBox::toggled,           this, &MeshSmoothingToolbar::minorSettingsChanged);

    // For these changes, we need to re-compute from scratch
    connect(rb_explicit,          &QRadioButton::toggled,        this, &MeshSmoothingToolbar::majorSettingsChanged);
    connect(rb_implicit,          &QRadioButton::toggled,        this, &MeshSmoothingToolbar::majorSettingsChanged);
    connect(rb_lap_cotan,         &QRadioButton::toggled,        this, &MeshSmoothingToolbar::majorSettingsChanged);
    connect(rb_lap_uniform,       &QRadioButton::toggled,        this, &MeshSmoothingToolbar::majorSettingsChanged);
    connect(dsb_timestep,         &QDoubleSpinBox::valueChanged, this, &MeshSmoothingToolbar::majorSettingsChanged);
    connect(cb_normalized_lap,    &QCheckBox::toggled,           this, &MeshSmoothingToolbar::majorSettingsChanged);

    connect(this, &MeshSmoothingToolbar::majorSettingsChanged, this, &MeshSmoothingToolbar::minorSettingsChanged);

    auto link_doublespinbox_and_slider = [this](QSlider *slider, QDoubleSpinBox *spinbox) {
        auto n_steps = (spinbox->maximum() - spinbox->minimum()) / spinbox->singleStep();
        slider->setMinimum(std::lround(spinbox->minimum()*n_steps));
        slider->setMaximum(std::lround(spinbox->maximum()*n_steps));
        slider->setValue(std::lround(spinbox->value()*n_steps));
        connect(spinbox,   &QDoubleSpinBox::valueChanged, [this,slider,n_steps](double v){
                slider->setSliderPosition(std::lround(v*n_steps));});
        connect(slider,   &QSlider::valueChanged, [this,spinbox,n_steps](int v){
                spinbox->setValue(v/n_steps);});
    };
    link_doublespinbox_and_slider(hs_timestep, dsb_timestep);
    link_doublespinbox_and_slider(hs_feat_alpha, dsb_feat_alpha);

}

LaplacianWeights MeshSmoothingToolbar::getLaplacianType() const {
    if (rb_lap_cotan->isChecked())   return LaplacianWeights::Cotan;
    if (rb_lap_uniform->isChecked()) return LaplacianWeights::Uniform;
    throw std::runtime_error("No laplacian selected?!");
}

SmoothingMethod MeshSmoothingToolbar::getSmoothingMethod() const {
    if (rb_explicit->isChecked()) return SmoothingMethod::Explicit;
    if (rb_implicit->isChecked()) return SmoothingMethod::Implicit;
    throw std::runtime_error("No smoothing method selected?!");
}
SmoothingSettings MeshSmoothingToolbar::getSettings() const
{
    SmoothingSettings settings;
    settings.method = getSmoothingMethod();
    settings.laplacian = getLaplacianType();
    settings.preserve_volume = cb_volume_preserve->isChecked();
    settings.timestep = std::pow(10.0, dsb_timestep->value());
    settings.iterations = sb_iters->value();
    settings.feature_enhance_alpha = cb_enhance->isChecked() ? dsb_feat_alpha->value() : 0.0;
    return settings;
}
