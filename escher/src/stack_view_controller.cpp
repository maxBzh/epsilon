extern "C" {
#include <assert.h>
}
#include <escher/stack_view_controller.h>
#include <escher/app.h>

StackViewController::ControllerView::ControllerView() :
  View(),
  m_contentView(nullptr),
  m_numberOfStacks(0),
  m_displayStackHeaders(true)
{
}

void StackViewController::ControllerView::shouldDisplayStackHearders(bool shouldDisplay) {
  m_displayStackHeaders = shouldDisplay;
}

void StackViewController::ControllerView::setContentView(View * view) {
  m_contentView = view;
  layoutSubviews();
  markRectAsDirty(bounds());
}

void StackViewController::ControllerView::pushStack(Frame frame) {
  m_stackViews[m_numberOfStacks].setNamedController(frame.viewController());
  m_stackViews[m_numberOfStacks].setTextColor(frame.textColor());
  m_stackViews[m_numberOfStacks].setBackgroundColor(frame.backgroundColor());
  m_stackViews[m_numberOfStacks].setSeparatorColor(frame.separatorColor());
  m_numberOfStacks++;
}

void StackViewController::ControllerView::popStack() {
  assert(m_numberOfStacks > 0);
  m_numberOfStacks--;
}

void StackViewController::ControllerView::layoutSubviews() {
  KDCoordinate width = m_frame.width();
  if (m_displayStackHeaders) {
    for (int i=0; i<m_numberOfStacks; i++) {
      m_stackViews[i].setFrame(KDRect(0, k_stackHeight*i, width, k_stackHeight + 1));
    }
  }
  if (m_contentView) {
    KDCoordinate separatorHeight = m_numberOfStacks > 0 ? 1 : 0;
    KDRect contentViewFrame = KDRect( 0,
        m_displayStackHeaders * (m_numberOfStacks * k_stackHeight + separatorHeight),
        width,
        m_frame.height() - m_displayStackHeaders * m_numberOfStacks * k_stackHeight);
    m_contentView->setFrame(contentViewFrame);
  }
}

int StackViewController::ControllerView::numberOfSubviews() const {
  return (m_displayStackHeaders ? m_numberOfStacks : 0) + (m_contentView == nullptr ? 0 : 1);
}

View * StackViewController::ControllerView::subviewAtIndex(int index) {
  if (!m_displayStackHeaders) {
    assert(index == 0);
    return m_contentView;
  }
  if (index < m_numberOfStacks) {
    assert(index >= 0);
    return &m_stackViews[index];
  } else {
    assert(index == m_numberOfStacks);
    return m_contentView;
  }
}

#if ESCHER_VIEW_LOGGING
const char * StackViewController::ControllerView::className() const {
  return "StackViewController::ControllerView";
}
#endif

StackViewController::StackViewController(Responder * parentResponder, ViewController * rootViewController, KDColor textColor, KDColor backgroundColor, KDColor separatorColor) :
  ViewController(parentResponder),
  m_view(),
  m_numberOfChildren(0),
  m_isVisible(false)
{
  pushModel(Frame(rootViewController, textColor, backgroundColor, separatorColor));
  rootViewController->setParentResponder(this);
}

const char * StackViewController::title() {
  ViewController * vc = m_childrenFrame[0].viewController();
  return vc->title();
}

void StackViewController::push(ViewController * vc, KDColor textColor, KDColor backgroundColor, KDColor separatorColor) {
  Frame frame = Frame(vc, textColor, backgroundColor, separatorColor);
  /* Add the frame to the model */
  pushModel(frame);
  if (!m_isVisible) {
    return;
  }
  /* Load stack view if the View Controller has a title. */
  if (vc->title() != nullptr && vc->displayParameter() != ViewController::DisplayParameter::DoNotShowOwnTitle) {
    m_view.pushStack(frame);
  }
  setupActiveViewController();
  if (m_numberOfChildren > 1) {
    m_childrenFrame[m_numberOfChildren-2].viewController()->viewDidDisappear();
  }
}

void StackViewController::pop() {
  assert(m_numberOfChildren > 0);
  ViewController * vc = m_childrenFrame[m_numberOfChildren-1].viewController();
  if (vc->title() != nullptr && vc->displayParameter() != ViewController::DisplayParameter::DoNotShowOwnTitle) {
    m_view.popStack();
  }
  m_numberOfChildren--;
  setupActiveViewController();
  vc->setParentResponder(nullptr);
  vc->viewDidDisappear();
}

int StackViewController::depth() {
  return m_numberOfChildren;
}

void StackViewController::pushModel(Frame frame) {
  m_childrenFrame[m_numberOfChildren++] = frame;
}

void StackViewController::setupActiveViewController() {
  ViewController * vc = m_childrenFrame[m_numberOfChildren-1].viewController();
  vc->setParentResponder(this);
  m_view.shouldDisplayStackHearders(vc->displayParameter() != ViewController::DisplayParameter::WantsMaximumSpace);
  m_view.setContentView(vc->view());
  vc->viewWillAppear();
  vc->setParentResponder(this);
  app()->setFirstResponder(vc);
}

void StackViewController::didBecomeFirstResponder() {
  ViewController * vc = m_childrenFrame[m_numberOfChildren-1].viewController();
  app()->setFirstResponder(vc);
}

bool StackViewController::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::Back && m_numberOfChildren > 1) {
    pop();
    return true;
  }
  return false;
}

View * StackViewController::view() {
  return &m_view;
}

void StackViewController::viewWillAppear() {
  /* Load the stack view */
  for (int i = 0; i < m_numberOfChildren; i++) {
    ViewController * childrenVC = m_childrenFrame[i].viewController();
    if (childrenVC->title() != nullptr && childrenVC->displayParameter() != ViewController::DisplayParameter::DoNotShowOwnTitle) {
      m_view.pushStack(m_childrenFrame[i]);
    }
  }
  /* Load the visible controller view */
  ViewController * vc = m_childrenFrame[m_numberOfChildren-1].viewController();
  if (m_numberOfChildren > 0 && vc) {
    m_view.setContentView(vc->view());
    m_view.shouldDisplayStackHearders(vc->displayParameter() != ViewController::DisplayParameter::WantsMaximumSpace);
    vc->viewWillAppear();
  }
  m_isVisible = true;
}

void StackViewController::viewDidDisappear() {
  ViewController * vc = m_childrenFrame[m_numberOfChildren-1].viewController();
  if (m_numberOfChildren > 0 && vc) {
    vc->viewDidDisappear();
  }
  for (int i = 0; i < m_view.numberOfStacks(); i++) {
    m_view.popStack();
  }
  m_isVisible = false;
}
