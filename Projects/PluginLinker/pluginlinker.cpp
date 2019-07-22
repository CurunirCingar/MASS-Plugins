#include "pluginlinker.h"

PluginLinker::PluginLinker() :
    QObject(nullptr),
    Service::CoreServiceBase(this),
    m_pluginUidCounter(0)
{
}

PluginLinker::~PluginLinker()
{
}

int PluginLinker::getCorePluginUID()
{
//    return m_PluginLinker.data()->getPluginUID();
}

QMap<int, QWeakPointer<IPluginLinker::ILinkerItem> > PluginLinker::getPluginsMap()
{
    QMap<int, QWeakPointer<IPluginLinker::ILinkerItem> > mapCopy;
    for(auto iter = m_linkerItemsMap.begin(); iter != m_linkerItemsMap.end(); ++iter)
    {
        mapCopy.insert(iter.key(), iter.value());
    }
    return mapCopy;
}

void PluginLinker::onServiceManagerInitialized()
{
    Service::CoreServiceBase::onServiceManagerInitialized();
}

bool PluginLinker::addCorePlugin(QWeakPointer<IPluginHandler> pluginHandler)
{
    auto linkerItem = createLinkerItem(pluginHandler);
    if(linkerItem.isNull())
    {
        qCritical() << "PluginLinker::addNewPlugin: core plugin adding failed";
        return false;
    }
    return true;
}

bool PluginLinker::addPlugin(QWeakPointer<IPluginHandler> pluginHandler)
{
    auto linkerItem = createLinkerItem(pluginHandler);
    if(linkerItem.isNull())
    {
        qCritical() << "PluginLinker::addNewPlugin: plugin adding failed";
        return false;
    }

    return true;
}

QSharedPointer<MetaInfo> PluginLinker::parseMetaInfo(const QJsonObject &metaInfoObject) const
{
    QJsonObject metaInfo = metaInfoObject.value("MetaData").toObject();
    // Check if all meta fields exists
    QStringList metaFieldsNames;
    metaFieldsNames << META_FIELD_INTERFACE <<
                    META_FIELD_NAME <<
                    META_FIELD_RELATED_PLUGIN_INTERFACES;

    for(QString metaFieldName : metaFieldsNames)
    {
        if(!metaInfo.contains(metaFieldName))
        {
//            log(SeverityType::WARNING, QString("PluginBase::parseMetaInfo: meta has no field '%1' but has fields:").arg(metaFieldName));
            for(auto iter = metaInfo.begin(); iter != metaInfo.end(); ++iter)
            {
//                log(SeverityType::WARNING, QString("%1: %2").arg(iter.key()).arg(iter.value().toString()));
            }
            return nullptr;
        }
    }

    QSharedPointer<MetaInfo> newMetaInfo(new MetaInfo());

    // Set module name
    newMetaInfo->Name = metaInfo.value(META_FIELD_NAME).toString();
    if(newMetaInfo->Name == "")
    {
//        log(SeverityType::CRITICAL, QString("PluginBase::parseMetaInfo: parse error: field %1 is empty.").arg(META_FIELD_NAME));
        return nullptr;
    }

    // Set module type
    newMetaInfo->InterfaceName = metaInfo.value(META_FIELD_INTERFACE).toString();
    if(newMetaInfo->InterfaceName == "")
    {
//        log(SeverityType::WARNING, QString("PluginBase::parseMetaInfo: plugin %1 field %2 is empty; "
//                                           "this item won't be referenced by other plugins.")
//                                .arg(newMetaInfo->Name)
//                                .arg(META_FIELD_INTERFACE));
    }

    // Set module parent name
    QJsonArray array = metaInfo.value(META_FIELD_RELATED_PLUGIN_INTERFACES).toArray();
    for(auto iter = array.begin(); iter != array.end(); ++iter)
    {
        newMetaInfo->RequiredInterfaces.append(iter->toString());
    }

    // Set module parent name
    auto jsonValueAbout = metaInfo.value(META_FIELD_ABOUT);
    newMetaInfo->About = jsonValueAbout.isNull() ? "" : jsonValueAbout.toString();

    //    qDebug() << "PluginBase::parseMetaInfo: succesfuly parsed:" <<
    //             META_FIELD_NAME << newMetaInfo->Name << endl <<
    //             META_FIELD_INTERFACE << newMetaInfo->InterfaceName << endl <<
    //             META_FIELD_RELATED_PLUGIN_INTERFACES << newMetaInfo->RelatedPluginNames << endl;
    return newMetaInfo;
}

QSharedPointer<LinkerItem> PluginLinker::createLinkerItem(QWeakPointer<IPluginHandler> pluginHandler)
{
    if(pluginHandler.isNull())
    {
//        log(SeverityType::WARNING, "PluginLinker::createLinkerItem: given hander is empty");
        return QWeakPointer<LinkerItem>();
    }

    auto jsonObject = pluginHandler.data()->getMeta();
    auto metaInfo = parseMetaInfo(jsonObject);

    if(metaInfo.isNull())
    {
//        log(SeverityType::WARNING, QString("PluginLinker::createLinkerItem: can't load plugin '%1'").arg(m_metaInfo.Name));
        return QWeakPointer<LinkerItem>();
    }

    auto &&linkerItem = new LinkerItem(pluginHandler, metaInfo, m_pluginUidCounter);
    auto linkerItemPtr = QSharedPointer<LinkerItem>(linkerItem);
    m_linkerItemsMap.insert(linkerItemPtr->getPluginUID(), linkerItemPtr);
    m_interfacesMap.insertMulti(metaInfo->InterfaceName, linkerItemPtr);
    ++m_pluginUidCounter;
//    log(SeverityType::INFO, QString("Plugin '%1' loaded").arg(metaInfo.data()->Name));
    return linkerItemPtr;
}

template<class Type>
Type *PluginLinker::castToPlugin(QObject *possiblePlugin) const
{
    Type *plugin = qobject_cast<Type *>(possiblePlugin);

    if(!plugin)
    {
//        log(SeverityType::WARNING, QString("Can't load the plugin '%1': not QObject.").arg(possiblePlugin->objectName()));
    }

    return plugin;
}

bool PluginLinker::setupLinks()
{
    bool isLinkageSucceded = true;
    QStringList troubledPlugins;
    // For all plugins.
    for(auto iter = m_interfacesMap.begin(); iter != m_interfacesMap.end(); ++iter)
    {
        auto &item = iter.value();
        auto &referencesNames = item->getReferenceNamesList();
        // For all references of plugin.
        for(auto &referenceName : referencesNames)
        {
            auto referenceIter = m_interfacesMap.find(referenceName);
            if(referenceIter != m_interfacesMap.end())
            {
                // For references with same interface.
                do
                {
                    item->addReference(referenceIter.value());
                    referenceIter.value()->addReferent(item);
                    ++referenceIter;
                }
                while(referenceIter.key() == referenceName && referenceIter != m_interfacesMap.end());
            }
            else
            {
                if(!troubledPlugins.contains(referenceName))
                {
                    troubledPlugins.append(referenceName);
                }
                isLinkageSucceded = false;
            }
        }
    }

    if(isLinkageSucceded)
    {
//        log(SeverityType::INFO, "Linkage suceeded");
    }
    else
    {
        //log(SeverityType::CRITICAL) << "Linkage failed, next referenced plugins not found:" << troubledPlugins;
    }

    for (auto iter = m_linkerItemsMap.begin(); iter != m_linkerItemsMap.end(); ++iter)
    {
        iter.value().data()->load();
    }

    onLinkageFinished();
    return isLinkageSucceded;
}

bool PluginLinker::unloadPlugin(QWeakPointer<ILinkerItem> linkerItem)
{
    auto uid = linkerItem.data()->getPluginUID();
    auto pluginHandler = m_linkerItemsMap[uid];
    pluginHandler.data()->unload();
    m_linkerItemsMap.remove(uid);
}

bool PluginLinker::loadPlugin(QString filename)
{
//    auto handler = m_app->makePluginHandler(filename);
//    return addPlugin(handler);
}