#include "GraphicsOptionsMenu.hpp"

#include <SmSdk/Gui/OptionsItemSlider.hpp>
#include <SmSdk/GameState.hpp>
#include <SmSdk/Util/Memory.hpp>

#include "Utils/Console.hpp"

class TimeScaleSlider : public OptionsItemSlider
{
public:
	inline TimeScaleSlider(MyGUI::Widget* pWidget) :
		OptionsItemSlider(pWidget, "Time Scale", GraphicsOptionsMenu::TimeScaleMin, GraphicsOptionsMenu::TimeScaleMax, 200)
	{
		this->update();

		m_pSlider->eventScrollChangePosition += MyGUI::newDelegate(
			this, &TimeScaleSlider::sliderChangePosition);
	}

	void updateText()
	{
		std::string text = std::to_string(this->getFraction());
		// Restrict to 2 decimal points
		size_t dotPos = text.find('.');
		if ( dotPos != std::string::npos && dotPos + 3 < text.size() )
			text = text.substr(0, dotPos + 3);
		m_pValueTextBox->setCaption(text);
	}

	void sliderChangePosition(MyGUI::ScrollBar* caller, std::size_t new_val)
	{
		GraphicsOptionsMenu::TimeScale = this->getFraction();
		this->updateText();
	}

	std::size_t getTimeScaleApprox() const
	{
		return std::size_t((GraphicsOptionsMenu::TimeScale / GraphicsOptionsMenu::TimeScaleMax) * float(m_uSteps));
	}

	void update() override
	{
		m_pSlider->setScrollPosition(this->getTimeScaleApprox());
		this->updateText();
	}
};

__int64 GraphicsOptionsMenu::h_CreateWidgets(GraphicsOptionsMenu* self, MyGUI::Widget* parent)
{
	const __int64 v_result = GraphicsOptionsMenu::o_CreateWidgets(self, parent);

	DebugOutL(__FUNCTION__, " -> Injected time scale settings");

	auto v_time_slider = std::make_shared<TimeScaleSlider>(
		self->m_leftStackBox.createNewOption());
	self->m_optionItems.emplace_back(std::move(v_time_slider));

	return v_result;
}
