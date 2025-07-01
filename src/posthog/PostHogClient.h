#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <queue>
#include <vector>
#include <set>
#include <memory>
#include <map>
#include "../ConfigManager.h"
#include "SystemController.h"
#include "EventQueue.h"
#include "parsers/InsightParser.h"
#include "../AsyncHTTPClient.h"

/**
 * @class PostHogClient
 * @brief Client for fetching PostHog insight data
 * 
 * Features:
 * - Queued insight requests with retry logic
 * - Automatic refresh of insights
 * - Thread-safe operation with event queue
 * - Configurable retry and refresh intervals
 * - Support for multiple insight types
 */
class PostHogClient {
public:
    /**
     * @brief Constructor
     * 
     * @param config Reference to configuration manager
     * @param eventQueue Reference to event system
     */
    explicit PostHogClient(ConfigManager& config, EventQueue& eventQueue);
    
    // Delete copy constructor and assignment operator
    PostHogClient(const PostHogClient&) = delete;
    void operator=(const PostHogClient&) = delete;
    
    /**
     * @brief Queue an insight for immediate fetch with Apple-like UX
     * 
     * @param insight_id ID of insight to fetch
     * @param forceRefresh If true, force recalculation instead of using cache
     * 
     * Uses AsyncNetworkManager for smooth loading states and progressive updates.
     * Shows cached data immediately if available, then updates with fresh data.
     */
    void requestInsightData(const String& insight_id, bool forceRefresh = false);
    
    /**
     * @brief Request insight data with cached data for progressive loading
     * 
     * @param insight_id ID of insight to fetch
     * @param cachedData Cached data to show immediately
     * @param forceRefresh If true, force recalculation instead of using cache
     */
    void requestInsightDataWithCache(const String& insight_id, const String& cachedData, bool forceRefresh = false);
    
    /**
     * @brief Check if client is ready for operation
     * 
     * @return true if configured and connected
     */
    bool isReady() const;
    
    /**
     * @brief Process queued requests and refreshes
     * 
     * Should be called regularly in main loop.
     * Handles:
     * - Processing queued requests
     * - Retrying failed requests
     * - Refreshing existing insights
     */
    void process();
    
private:
    /**
     * @struct QueuedRequest
     * @brief Tracks a queued insight request
     */
    struct QueuedRequest {
        String insight_id;     ///< ID of insight to fetch
        uint8_t retry_count;   ///< Number of retry attempts
        bool force_refresh;    ///< Force recalculation instead of cache
    };
    
    // Configuration
    ConfigManager& _config;         ///< Configuration storage
    EventQueue& _eventQueue;        ///< Event system
    
    // Async network management
    std::unique_ptr<AsyncHTTPClient> _asyncHttpClient; ///< Truly async HTTP client
    
    // Request tracking
    std::set<String> requested_insights;  ///< All known insight IDs
    std::queue<QueuedRequest> request_queue; ///< Queue of pending requests (legacy)
    bool has_active_request;               ///< Request in progress flag (legacy)
    WiFiClientSecure _secureClient;        ///< Secure WiFi client for HTTPS
    HTTPClient _http;                      ///< HTTP client instance
    unsigned long last_refresh_check;       ///< Last refresh timestamp
    
    // Data caching for progressive loading
    std::map<String, String> _insightCache; ///< Cached insight data
    std::map<String, unsigned long> _cacheTimestamps; ///< Cache timestamps
    
    // Constants
    static const char* BASE_URL;                        ///< PostHog API base URL
    static const unsigned long REFRESH_INTERVAL = 30000; ///< Refresh every 30s
    static const uint8_t MAX_RETRIES = 3;              ///< Max retry attempts
    static const unsigned long RETRY_DELAY = 1000;      ///< Delay between retries
    


        /**
     * @brief Build Base API URL based on project region
     */
    String buildBaseUrl() const;

    /**
     * @brief Handle system state changes
     * @param state New system state
     */
    void onSystemStateChange(SystemState state);
    
    /**
     * @brief Process pending requests in queue
     * 
     * Handles retry logic and request timeouts.
     */
    void processQueue();
    
    /**
     * @brief Check if insights need refreshing
     * 
     * Queues refresh requests for insights older than REFRESH_INTERVAL.
     */
    void checkRefreshes();
    
    /**
     * @brief Fetch insight data from PostHog
     * 
     * @param insight_id ID of insight to fetch
     * @param response String to store response
     * @param forceRefresh If true, force recalculation instead of using cache
     * @return true if fetch was successful
     */
    bool fetchInsight(const String& insight_id, String& response, bool forceRefresh = false);
    
    /**
     * @brief Build insight API URL
     * 
     * @param insight_id ID of insight
     * @param refresh_mode Cache control mode
     * @return Complete API URL
     */
    String buildInsightUrl(const String& insight_id, const char* refresh_mode = "force_cache") const;
    
    // Event-related methods
    void publishInsightDataEvent(const String& insight_id, const String& response);
    
    /**
     * @brief Make async insight request
     * @param insight_id ID of insight to fetch
     * @param forceRefresh Whether to force refresh
     */
    void makeAsyncInsightRequest(const String& insight_id, bool forceRefresh);
    
    /**
     * @brief Handle successful insight data retrieval
     * @param insight_id ID of insight
     * @param data Retrieved data
     * @param statusCode HTTP status code
     */
    void handleInsightSuccess(const String& insight_id, const String& data, int statusCode);
    
    /**
     * @brief Handle insight data retrieval error
     * @param insight_id ID of insight
     * @param error Error message
     * @param statusCode HTTP status code
     */
    void handleInsightError(const String& insight_id, const String& error, int statusCode);
    
    /**
     * @brief Get cached data for an insight
     * @param insight_id ID of insight
     * @return Cached data or empty string if not available
     */
    String getCachedData(const String& insight_id) const;
    
    /**
     * @brief Cache insight data
     * @param insight_id ID of insight
     * @param data Data to cache
     */
    void cacheInsightData(const String& insight_id, const String& data);
    
    /**
     * @brief Check if cached data is still valid
     * @param insight_id ID of insight
     * @return true if cache is valid
     */
    bool isCacheValid(const String& insight_id) const;
}; 