class ViewTest : public EnvGraph::TestBase
{
  protected:
    void SetUp() override
    {
        EnvGraph::TestBase::SetUp();

        vc.NewView(EnvGraph::ViewType::LOCAL_LINUX, view);

#ifdef _WIN32
        view.SetWin32(m_window);
#else
        view.SetXCB(m_xConnection);
#endif
    };

    void TearDown() override
    {
        ASSERT_EQ(vc.CloseView(view), 0);
        EnvGraph::TestBase::TearDown();
    };

  private:
    ViewController vc;
    View view;
};