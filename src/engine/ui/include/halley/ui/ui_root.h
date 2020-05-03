#pragma once
#include "halley/time/halleytime.h"
#include "halley/maths/vector2.h"
#include "ui_event.h"
#include "halley/core/input/input_virtual.h"
#include "ui_parent.h"
#include "ui_input.h"
#include "halley/core/api/audio_api.h"
#include "halley/core/game/core.h"
#include "halley/core/input/input_keyboard.h"

namespace Halley {
	class SpritePainter;
	class AudioAPI;
	class AudioClip;
	class TextInputCapture;

	enum class UIInputType {
		Undefined,
		Mouse,
		Keyboard,
		Gamepad
	};
	
	class UIRoot final : public UIParent {
	public:
		explicit UIRoot(const HalleyAPI& api, Rect4f rect = {});
		~UIRoot();

		UIRoot* getRoot() override;
		const UIRoot* getRoot() const override;
		const String& getId() const override;

		void setRect(Rect4f rect, Vector2f overscan = Vector2f());
		Rect4f getRect() const override;

		void update(Time t, UIInputType activeInputType, spInputDevice mouse, spInputDevice manual);
		void draw(SpritePainter& painter, int mask, int layer);
		void render(RenderContext& rc);

		void mouseOverNext(bool forward = true);
		void runLayout();
		
		std::optional<AudioHandle> playSound(const String& eventName);
		void sendEvent(UIEvent event) const override;

		bool hasModalUI() const;
		bool isMouseOverUI() const;
		std::shared_ptr<UIWidget> getWidgetUnderMouse() const;
		std::shared_ptr<UIWidget> getWidgetUnderMouseIncludingDisabled() const;
		void setFocus(const std::shared_ptr<UIWidget>& newFocus);
		void focusNext(bool reverse);

		UIWidget* getCurrentFocus() const;

		void setUIMouseRemapping(std::function<Vector2f(Vector2f)> remapFunction);
		void unsetUIMouseRemapping();

		std::vector<std::shared_ptr<UIWidget>> collectWidgets();

		void onChildAdded(UIWidget& child) override;

		void registerKeyPressListener(std::shared_ptr<UIWidget> widget, int priority = 0);
				
	private:
		String id;
		std::shared_ptr<InputKeyboard> keyboard;
		AudioAPI* audio = nullptr;
		Rect4f uiRect;

		std::weak_ptr<UIWidget> currentMouseOver;
		std::weak_ptr<UIWidget> mouseExclusive; // A widget that's taking exclusive control of mouse
		std::weak_ptr<UIWidget> currentFocus;
		Vector2f lastMousePos;
		std::shared_ptr<InputDevice> dummyInput;
		Vector2f overscan;

		bool anyMouseButtonHeld = false;

		std::function<Vector2f(Vector2f)> mouseRemap;
		std::unique_ptr<TextInputCapture> textCapture;
		std::vector<std::pair<std::weak_ptr<UIWidget>, int>> keyPressListeners;

		void updateMouse(const spInputDevice& mouse);
		void updateGamepadInputTree(const spInputDevice& input, UIWidget& c, std::vector<UIWidget*>& inputTargets, UIGamepadInput::Priority& bestPriority, bool accepting);
		void updateGamepadInput(const spInputDevice& input);

		void updateKeyboardInput();
		void sendKeyPress(KeyboardKeyPress key);
		void onUnhandledKeyPress(KeyboardKeyPress key);
		void receiveKeyPress(KeyboardKeyPress key) override;

		std::shared_ptr<UIWidget> getWidgetUnderMouse(Vector2f mousePos, bool includeDisabled = false) const;
		std::shared_ptr<UIWidget> getWidgetUnderMouse(const std::shared_ptr<UIWidget>& start, Vector2f mousePos, bool includeDisabled = false) const;
		void updateMouseOver(const std::shared_ptr<UIWidget>& underMouse);
		void collectWidgets(const std::shared_ptr<UIWidget>& start, std::vector<std::shared_ptr<UIWidget>>& output);

		void focusWidget(UIWidget& widget);
		void unfocusWidget(UIWidget& widget);
	};
}
