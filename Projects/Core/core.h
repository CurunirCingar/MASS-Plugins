#ifndef COREPLUGIN_H
#define COREPLUGIN_H

#include <QObject>
#include <QDebug>
#include <QString>
#include <typeindex>

#include "../../../Application/icoreplugin.h"
#include "../../Interfaces/Architecture/iplugin.h"
#include "../../Interfaces/Architecture/PluginBase/plugin_base.h"

#include "simplelinker.h"


//! addtogroup CorePlugin_imp
//! {
class Core : public QObject, public ICore, public PluginBase, public IApplication
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "MASS.Module.CorePlugin" FILE "PluginMeta.json")
    Q_INTERFACES(
            ICore
            IPlugin
            IApplication)

public:
    Core();
    virtual ~Core() override;

    // ICore interface
public:
    virtual void coreInit(IApplication *app) override;
    virtual bool coreFini() override;

    // IApplication interface
public:
    virtual QWidget *getParentWidget() override;
    virtual QVector<IPluginHandlerPtr > getPlugins() override;
    virtual IPluginHandlerPtr makePluginHandler(QString path) override;

 private:
    IApplication* m_app;
    Service::SimpleLinker* m_linker;
};
//! }
#endif // COREPLUGIN_H
