#include "shiftbuttonconfig.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>

#include "buttonlogical.h"
#include "global.h"
#include "deviceconfig.h"

ShiftButtonConfig::ShiftButtonConfig(QWidget *parent)
    : QWidget(parent)
{
    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(8, 8, 8, 8);
    lay->setSpacing(4);

    auto *hint = new QLabel(
        tr("A shift layer is held while its button is active. Shift buttons are "
           "configured like normal buttons but never appear as joystick buttons "
           "— use Toggle for a latching shift-lock, or Logic to combine inputs. "
           "Note: a Sequential-toggle shift always keeps one layer active, so a "
           "button with no shift assigned is suppressed while such a shift exists."),
        this);
    hint->setWordWrap(true);
    // Keep the hint from spanning the whole tab (the shift table only uses the
    // left portion) -- cap it near the table width so it wraps above the columns.
    hint->setMaximumWidth(560);
    hint->setStyleSheet(QStringLiteral("color: palette(mid); font-style: italic;"));
    lay->addWidget(hint);

    /* Column header, pixel-aligned to the ButtonLogical row columns below by
     * reusing the exact fixed widths from the Button Config "Logical buttons"
     * header (buttonconfig.ui / buttonlogical.ui), so the shift table reads as the
     * same component. Order matches a shift row's VISIBLE columns only -- the
     * drag-handle, Shift and Delay/Press timer columns are hidden on shift rows
     * (setTarget), so they're omitted here. Trailing stretch mirrors the row's
     * right-hand whitespace. */
    auto *header = new QWidget(this);
    auto *hlay = new QHBoxLayout(header);
    hlay->setContentsMargins(0, 0, 0, 0);
    hlay->setSpacing(2);
    auto addHead = [&](int w, const QString &text) {
        auto *l = new QLabel(text, header);
        l->setFixedWidth(w);
        l->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
        hlay->addWidget(l);
    };
    addHead(26, QString());          // S<n> label column
    addHead(26, QString());          // listen / target button
    addHead(50, tr("Physical"));
    addHead(28, QString());          // disable checkbox
    addHead(2,  QString());          // inter-checkbox spacer
    addHead(28, QString());          // invert checkbox
    addHead(145, tr("Function"));
    addHead(90, tr("Operator"));
    addHead(26, QString());          // Source B listen / target
    addHead(50, tr("Source B"));
    hlay->addStretch(1);
    lay->addWidget(header);

    for (int i = 0; i < MAX_SHIFTS_NUM; ++i) {
        auto *row = new ButtonLogical(i, this);
        lay->addWidget(row);
        m_rows.append(row);
    }
    // Pin the rows to the top; the stretch collapses if they ever overflow.
    lay->addStretch(1);

    // Build each row's dropdowns, THEN switch it to shift mode (setTarget's
    // type filter needs the Function dropdown populated first). Deferred to the
    // event loop -- like ButtonConfig::logicaButtonsCreator -- because doing the
    // grouped-combobox setup during MainWindow construction (before the event
    // loop, before the widget is shown) hangs the UI.
    QTimer::singleShot(0, this, [this] {
        for (ButtonLogical *row : m_rows) {
            row->initialization();
            row->setTarget(ButtonLogical::ShiftButtons);
        }
    });
}

void ShiftButtonConfig::readFromConfig()
{
    for (ButtonLogical *row : m_rows) {
        row->readFromConfig();
    }
}

void ShiftButtonConfig::writeToConfig()
{
    for (ButtonLogical *row : m_rows) {
        row->writeToConfig();
    }
}

void ShiftButtonConfig::setUiOnOff(int value)
{
    // value == total physical buttons (0 when disconnected). Set the per-row
    // physical-button spinbox max and enabled state, same as ButtonConfig.
    for (ButtonLogical *row : m_rows) {
        row->setMaxPhysButtons(value);
        row->setSpinBoxOnOff(value);
    }
}

void ShiftButtonConfig::shiftStateChanged()
{
    if (gEnv.pDeviceConfig == nullptr) {
        return;
    }
    const params_report_t *pr = &gEnv.pDeviceConfig->paramsReport;
    for (int i = 0; i < m_rows.size() && i < MAX_SHIFTS_NUM; ++i) {
        m_rows[i]->setButtonState((pr->shift_button_data & (1 << i)) != 0);
    }
}
