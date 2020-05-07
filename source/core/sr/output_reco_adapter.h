//
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.md file in the project root for full license information.
//
#pragma once

#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>

#include "spxcore_common.h"
#include "ispxinterfaces.h"
#include "interface_helpers.h"

namespace Microsoft {
namespace CognitiveServices {
namespace Speech {
namespace Impl {

class CSpxOutputRecoEngineAdapter final:
    public ISpxObjectWithSiteInitImpl<ISpxRecoEngineAdapterSite>,
    public ISpxRecoEngineAdapter,
    public ISpxAudioDataStream,
    public ISpxRetrievable
{
public:
    CSpxOutputRecoEngineAdapter() = default;
    ~CSpxOutputRecoEngineAdapter() = default;

    SPX_INTERFACE_MAP_BEGIN()
        SPX_INTERFACE_MAP_ENTRY(ISpxObjectWithSite)
        SPX_INTERFACE_MAP_ENTRY(ISpxObjectInit)
        SPX_INTERFACE_MAP_ENTRY(ISpxRecoEngineAdapter)
        SPX_INTERFACE_MAP_ENTRY(ISpxAudioProcessor)
        SPX_INTERFACE_MAP_ENTRY(ISpxAudioDataStream)
        SPX_INTERFACE_MAP_ENTRY(ISpxRetrievable)
    SPX_INTERFACE_MAP_END()

    // --- ISpxObject
    inline void Init() final {};
    inline void Term() final {};

    // --- ISpxRecoEngineAdapter
    void SetAdapterMode(bool singleShot) final;

    // --- ISpxAudioProcessor
    /* For tomorrow... SetFormat should init the stream, term should let the site know that we are done */
    void SetFormat(const SPXWAVEFORMATEX* format) final;
    void ProcessAudio(const DataChunkPtr& audioChunk) final;

    // -- ISpxRetrievable
    inline void MarkAsRetrieved() noexcept final
    {
        m_retrieved = true;
    }

    inline bool WasRetrieved() const noexcept final
    {
        return m_retrieved;
    }

    // -- ISpxAudioDataStream
    inline void InitFromSynthesisResult(std::shared_ptr<ISpxSynthesisResult>) final
    {}

    inline void InitFromFormat(const SPXWAVEFORMATEX& format, bool hasHeader) final
    {
        m_stream->InitFromFormat(format, hasHeader);
    }

    inline StreamStatus GetStatus() noexcept final
    {
        return m_status;
    }

    inline CancellationReason GetCancellationReason() final
    {
        return m_stream->GetCancellationReason();
    }

    inline CancellationErrorCode GetCancellationErrorCode() final
    {
        return m_stream->GetCancellationErrorCode();
    }

    inline bool CanReadData(uint32_t requestedSize) final
    {
        WaitForStateNot(StreamStatus::NoData);
        return m_stream->CanReadData(requestedSize);
    }

    inline bool CanReadData(uint32_t requestedSize, uint32_t pos) final
    {
        WaitForStateNot(StreamStatus::NoData);
        return m_stream->CanReadData(requestedSize, pos);
    }

    inline uint32_t Read(uint8_t* buffer, uint32_t bufferSize) final
    {
        WaitForStateNot(StreamStatus::NoData);
        return m_stream->Read(buffer, bufferSize);
    }

    inline uint32_t Read(uint8_t* buffer, uint32_t bufferSize, uint32_t pos) final
    {
        WaitForStateNot(StreamStatus::NoData);
        return m_stream->Read(buffer, bufferSize, pos);
    }

    inline void SaveToWaveFile(const wchar_t* fileName) final
    {
        if (GetStatus() != StreamStatus::AllData)
        {
            SPX_THROW_HR(SPXERR_INVALID_STATE);
        }
        m_stream->SaveToWaveFile(fileName);
    }

    inline uint32_t GetPosition() final
    {
        return m_stream->GetPosition();
    }

    inline void SetPosition(uint32_t pos) final
    {
        return m_stream->SetPosition(pos);
    }

    void DetachInput() final;
private:
    inline void WaitForState(StreamStatus status) const
    {
        std::unique_lock<std::mutex> lk{ m_stateMutex };
        m_cv.wait(lk, [this, status]()
        {
            return m_status == status;
        });
    }

    inline void WaitForStateNot(StreamStatus status) const
    {
        std::unique_lock<std::mutex> lk{ m_stateMutex };
        m_cv.wait(lk, [this, status]()
        {
            return m_status != status;
        });
    }

    using SitePtr = std::shared_ptr<ISpxRecoEngineAdapterSite>;
    StreamStatus m_status{ StreamStatus::NoData };

    std::shared_ptr<ISpxAudioDataStream> m_stream;
    std::shared_ptr<ISpxAudioOutput> m_sink;

    std::atomic<bool> m_detaching{ false };
    bool m_retrieved{ false };
    mutable std::condition_variable m_cv;
    mutable std::mutex m_stateMutex;

};

} } } }