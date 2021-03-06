/*
 * Copyright (C) 2011 Emweb bvba, Kessel-Lo, Belgium.
 *
 * See the LICENSE file for terms of use.
 */

#include "Wt/WApplication.h"
#include "Wt/WAbstractToggleButton.h"
#include "Wt/WFormWidget.h"
#include "Wt/WLogger.h"
#include "Wt/WText.h"
#include "Wt/WTemplateFormView.h"
#include "Wt/WTheme.h"

#include "WebUtils.h"

namespace Wt {

  LOGGER("WTemplateFormView");

WTemplateFormView::FieldData::FieldData()
  : formWidget(nullptr)
{ }

WTemplateFormView::WTemplateFormView(const WString& text)
  : WTemplate(text)
{
  init();
}

WTemplateFormView::WTemplateFormView()
{
  init();
}

void WTemplateFormView::init()
{
  addFunction("id", &Functions::id);
  addFunction("tr", &Functions::tr);
  addFunction("block", &Functions::block);
}

void WTemplateFormView::setFormWidget(WFormModel::Field field,
				      std::unique_ptr<Wt::WWidget> formWidget)
{
  fields_[field] = FieldData();
  fields_[field].formWidget = formWidget.get();
  bindWidget(field, std::move(formWidget));
}

#ifndef WT_TARGET_JAVA

void WTemplateFormView
::setFormWidget(WFormModel::Field field, std::unique_ptr<WWidget> formWidget,
		const std::function<void()>& updateViewValue,
		const std::function<void()>& updateModelValue)
{
  fields_[field].formWidget = formWidget.get();
  fields_[field].updateView = updateViewValue;
  fields_[field].updateModel = updateModelValue;

  bindWidget(field, std::move(formWidget));
}

#else

void WTemplateFormView
::setFormWidget(WFormModel::Field field, WWidget *formWidget,
		FieldView *fieldView)
{
  fields_[field] = FieldData();
  fields_[field].formWidget = formWidget;
  fields_[field].updateFunctions = fieldView;

  bindWidget(field, formWidget); 
}

#endif // WT_TARGET_JAVA

std::unique_ptr<WWidget> WTemplateFormView
::createFormWidget(WFormModel::Field field)
{
  return std::unique_ptr<WWidget>();
}

void WTemplateFormView::updateViewValue(WFormModel *model,
					WFormModel::Field field,
					WFormWidget *edit)
{
  if (updateViewValue(model, field, (WWidget *)edit))
    return;

  WAbstractToggleButton *b = dynamic_cast<WAbstractToggleButton *>(edit);
  if (b) {
    cpp17::any v = model->value(field);
    if (v.empty() || cpp17::any_cast<bool>(v) == false)
      b->setChecked(false);
    else
      b->setChecked(true);
  } else
    edit->setValueText(model->valueText(field));
}

bool WTemplateFormView::updateViewValue(WFormModel *model,
					WFormModel::Field field,
					WWidget *edit)
{
  FieldMap::const_iterator fi = fields_.find(field);

  if (fi != fields_.end()) {
#ifndef WT_TARGET_JAVA
    if (fi->second.updateView) {
      fi->second.updateView();
      return true;
    }
#else
    if (fi->second.updateFunctions) {
      fi->second.updateFunctions->updateViewValue();
      return true;
    }
#endif
  }

  return false;
} 

void WTemplateFormView::updateViewField(WFormModel *model,
					WFormModel::Field field)
{
  const std::string var = field;

  if (model->isVisible(field)) {
    setCondition("if:" + var, true);
    WWidget *edit = resolveWidget(var);
    if (!edit) {
      std::unique_ptr<WWidget> nw = createFormWidget(field);
      edit = nw.get();
      if (!edit) {
	LOG_ERROR("updateViewField: createFormWidget('"
		  << field << "') returned 0");
	return;
      }
      bindWidget(var, std::move(nw));
    }

    WFormWidget *fedit = dynamic_cast<WFormWidget *>(edit);
    if (fedit) {
      if (fedit->validator() != model->validator(field) &&
	  model->validator(field))
	fedit->setValidator(model->validator(field));
      updateViewValue(model, field, fedit);
    } else
      updateViewValue(model, field, edit);

    WText *info = resolve<WText *>(var + "-info");
    if (!info) {
      info = new WText();
      bindWidget(var + "-info", std::unique_ptr<WWidget>(info));
    }

    bindString(var + "-label", model->label(field));

    const WValidator::Result& v = model->validation(field);
    info->setText(v.message());
    indicateValidation(field, model->isValidated(field),
		       info, edit, v);
    edit->setDisabled(model->isReadOnly(field));
  } else {
    setCondition("if:" + var, false);
    bindEmpty(var);
    bindEmpty(var + "-info");    
  }
}

void WTemplateFormView::indicateValidation(WFormModel::Field field,
					   bool validated,
					   WText *info,
					   WWidget *edit,
					   const WValidator::Result& validation)
{
  info->setText(validation.message());

  if (validated) {
    WApplication::instance()->theme()
      ->applyValidationStyle(edit, validation, ValidationAllStyles);

    info->toggleStyleClass("Wt-error", 
			   validation.state() != ValidationState::Valid,
			   true);
  } else {
    WApplication::instance()->theme()
      ->applyValidationStyle(edit, validation, None);

    info->removeStyleClass("Wt-error", true);
  }
}

void WTemplateFormView::updateModelField(WFormModel *model,
					 WFormModel::Field field)
{
  WWidget *edit = resolveWidget(field);
  WFormWidget *fedit = dynamic_cast<WFormWidget *>(edit);
  if (fedit)
    updateModelValue(model, field, fedit);
  else
    updateModelValue(model, field, edit);
}

void WTemplateFormView::updateModelValue(WFormModel *model,
					 WFormModel::Field field,
					 WFormWidget *edit)
{
  if (updateModelValue(model, field, (WWidget *)edit))
    return;

  WAbstractToggleButton *b = dynamic_cast<WAbstractToggleButton *>(edit);
  if (b)
    model->setValue(field, b->isChecked());
  else
    model->setValue(field, edit->valueText());
}

bool WTemplateFormView::updateModelValue(WFormModel *model,
					 WFormModel::Field field,
					 WWidget *edit)
{
  FieldMap::const_iterator fi = fields_.find(field);

  if (fi != fields_.end()) {
#ifndef WT_TARGET_JAVA
    if (fi->second.updateModel) {
      fi->second.updateModel();
      return true;
    }
#else
    if (fi->second.updateFunctions) {
      fi->second.updateFunctions->updateModelValue();
      return true;
    }
#endif
  }
   
  return false;
}

void WTemplateFormView::updateModel(WFormModel *model)
{
  std::vector<WFormModel::Field> fields = model->fields();

  for (unsigned i = 0; i < fields.size(); ++i) {
    WFormModel::Field field = fields[i];
    updateModelField(model, field);
  }
}

void WTemplateFormView::updateView(WFormModel *model)
{
  std::vector<WFormModel::Field> fields = model->fields();

  for (unsigned i = 0; i < fields.size(); ++i) {
    WFormModel::Field field = fields[i];
    updateViewField(model, field);
  }
}

}
