// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include <boost/filesystem/path.hpp>
#include "zkfacade.h"
#include "zkfiledbmodel.h"

namespace filedistribution {

const std::string readApplicationId(ZKFacade & zk, const boost::filesystem::path & deployNode);

class DeployedFilesToDownload {
    //includes the current deploy run;
    static const size_t numberOfDeploysToKeepFiles = 2;
    typedef boost::filesystem::path Path;

    ZKFacade& _zk;

    Path getPath(const std::string& hostName) {
        return  ZKFileDBModel::_hostsPath / hostName;
    }

    //Nothrow
    template <typename INSERT_ITERATOR>
    void readDeployFile(const boost::filesystem::path& path, INSERT_ITERATOR insertionIterator);
    void addAppIdToDeployNode(const Path & deployNode, const std::string & appId);
    std::map<std::string, std::vector<std::string> > groupChildrenByAppId(const Path & parentPath, const std::vector<std::string> & children);
    void deleteExpiredDeployNodes(Path parentPath, std::vector<std::string> children);

public:
    typedef std::vector<std::string> FileReferences;

    DeployedFilesToDownload(ZKFacade* zk)
        :_zk(*zk)
    {}

    const Path addNewDeployNode(Path parentPath, const FileReferences& files);

    void deleteExpiredDeployNodes(Path parentPath);

    void setDeployedFilesToDownload(
        const std::string& hostName,
        const std::string& applicationId,
        const FileReferences& files);

    /** For all the deploys available **/
    const FileReferences getDeployedFilesToDownload(
        const std::string& hostName,
        const ZKFacade::NodeChangedWatcherSP& watcher);

    /** For the current deploy only **/
    const FileReferences getLatestDeployedFilesToDownload(const std::string& hostName);
};

} //namespace filedistribution

