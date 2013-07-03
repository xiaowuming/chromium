// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_TRANSLATE_TRANSLATE_HELPER_H_
#define CHROME_RENDERER_TRANSLATE_TRANSLATE_HELPER_H_

#include <string>

#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "chrome/common/translate/translate_errors.h"
#include "content/public/renderer/render_view_observer.h"

namespace WebKit {
class WebDocument;
class WebFrame;
}

// This class deals with page translation.
// There is one TranslateHelper per RenderView.

class TranslateHelper : public content::RenderViewObserver {
 public:
  explicit TranslateHelper(content::RenderView* render_view);
  virtual ~TranslateHelper();

  // Informs us that the page's text has been extracted.
  void PageCaptured(int page_id, const string16& contents);

 protected:
  // The following methods are protected so they can be overridden in
  // unit-tests.
  void OnTranslatePage(int page_id,
                       const std::string& translate_script,
                       const std::string& source_lang,
                       const std::string& target_lang);
  void OnRevertTranslation(int page_id);

  // Returns true if the translate library is available, meaning the JavaScript
  // has already been injected in that page.
  virtual bool IsTranslateLibAvailable();

  // Returns true if the translate library has been initialized successfully.
  virtual bool IsTranslateLibReady();

  // Returns true if the translation script has finished translating the page.
  virtual bool HasTranslationFinished();

  // Returns true if the translation script has reported an error performing the
  // translation.
  virtual bool HasTranslationFailed();

  // Starts the translation by calling the translate library.  This method
  // should only be called when the translate script has been injected in the
  // page.  Returns false if the call failed immediately.
  virtual bool StartTranslation();

  // Asks the Translate element in the page what the language of the page is.
  // Can only be called if a translation has happened and was successful.
  // Returns the language code on success, an empty string on failure.
  virtual std::string GetOriginalPageLanguage();

  // Adjusts a delay time for a posted task. This is used in tests to do tasks
  // immediately by returning 0.
  virtual base::TimeDelta AdjustDelay(int delayInMs);

  // Executes the JavaScript code in |script| in the main frame of RenderView.
  virtual void ExecuteScript(const std::string& script);

  // Executes the JavaScript code in |script| in the main frame of RenderView,
  // and returns the boolean returned by the script evaluation if the script was
  // run successfully. Otherwise, returns |fallback| value.
  virtual bool ExecuteScriptAndGetBoolResult(const std::string& script,
                                             bool fallback);

  // Executes the JavaScript code in |script| in the main frame of RenderView,
  // and returns the string returned by the script evaluation if the script was
  // run successfully. Otherwise, returns empty string.
  virtual std::string ExecuteScriptAndGetStringResult(
      const std::string& script);

  // Executes the JavaScript code in |script| in the main frame of RenderView.
  // and returns the number returned by the script evaluation if the script was
  // run successfully. Otherwise, returns 0.0.
  virtual double ExecuteScriptAndGetDoubleResult(const std::string& script);

 private:
  FRIEND_TEST_ALL_PREFIXES(TranslateHelperTest, IsValidLanguageCode);
  FRIEND_TEST_ALL_PREFIXES(TranslateHelperTest, AdoptHtmlLang);
  FRIEND_TEST_ALL_PREFIXES(TranslateHelperTest,
                           CLDAgreeWithLanguageCodeHavingCountryCode);
  FRIEND_TEST_ALL_PREFIXES(TranslateHelperTest,
                           CLDDisagreeWithWrongLanguageCode);
  FRIEND_TEST_ALL_PREFIXES(TranslateHelperTest,
                           InvalidLanguageMetaTagProviding);
  FRIEND_TEST_ALL_PREFIXES(TranslateHelperTest, LanguageCodeTypoCorrection);
  FRIEND_TEST_ALL_PREFIXES(TranslateHelperTest, LanguageCodeSynonyms);
  FRIEND_TEST_ALL_PREFIXES(TranslateHelperTest, ResetInvalidLanguageCode);
  FRIEND_TEST_ALL_PREFIXES(TranslateHelperTest, SimilarLanguageCode);
  FRIEND_TEST_ALL_PREFIXES(TranslateHelperTest, WellKnownWrongConfiguration);

  // Corrects language code if it contains well-known mistakes.
  static void CorrectLanguageCodeTypo(std::string* code);

  // Converts language code to the one used in server supporting list.
  static void ConvertLanguageCodeSynonym(std::string* code);

  // Checks if the language code's format is valid.
  static bool IsValidLanguageCode(const std::string& code);

  // Applies a series of language code modification in proper order.
  static void ApplyLanguageCodeCorrection(std::string* code);

  // Checks if languages are matched, or similar. This function returns true
  // against a language pair containing a language which is difficult for CLD
  // to distinguish.
  static bool IsSameOrSimilarLanguages(const std::string& page_language,
                                       const std::string& cld_language);

  // Checks if languages pair is one of well-known pairs of wrong server
  // configuration.
  static bool MaybeServerWrongConfiguration(const std::string& page_language,
                                            const std::string& cld_language);

  // Checks if CLD can complement a sub code when the page language doesn't
  // know the sub code.
  static bool CanCLDComplementSubCode(const std::string& page_language,
                                      const std::string& cld_language);

  // Determines content page language from Content-Language code and contents.
  static std::string DeterminePageLanguage(const std::string& code,
                                           const std::string& html_lang,
                                           const string16& contents,
                                           std::string* cld_language,
                                           bool* is_cld_reliable);

  // Returns whether the page associated with |document| is a candidate for
  // translation.  Some pages can explictly specify (via a meta-tag) that they
  // should not be translated.
  static bool IsTranslationAllowed(WebKit::WebDocument* document);

#if defined(ENABLE_LANGUAGE_DETECTION)
  // Returns the ISO 639_1 language code of the specified |text|, or 'unknown'
  // if it failed.
  // |is_cld_reliable| will be set as true if CLD says the detection is
  // reliable.
  static std::string DetermineTextLanguage(const string16& text,
                                           bool* is_cld_reliable);
#endif

  // RenderViewObserver implementation.
  virtual bool OnMessageReceived(const IPC::Message& message) OVERRIDE;

  // Cancels any translation that is currently being performed.  This does not
  // revert existing translations.
  void CancelPendingTranslation();

  // Checks if the current running page translation is finished or errored and
  // notifies the browser accordingly.  If the translation has not terminated,
  // posts a task to check again later.
  void CheckTranslateStatus();

  // Called by TranslatePage to do the actual translation.  |count| is used to
  // limit the number of retries.
  void TranslatePageImpl(int count);

  // Sends a message to the browser to notify it that the translation failed
  // with |error|.
  void NotifyBrowserTranslationFailed(TranslateErrors::Type error);

  // Convenience method to access the main frame.  Can return NULL, typically
  // if the page is being closed.
  WebKit::WebFrame* GetMainFrame();

  // ID to represent a page which TranslateHelper captured and determined a
  // content language.
  int page_id_;

  // The states associated with the current translation.
  bool translation_pending_;
  std::string source_lang_;
  std::string target_lang_;

  // Time when a page langauge is determined. This is used to know a duration
  // time from showing infobar to requesting translation.
  base::TimeTicks language_determined_time_;

  // Method factory used to make calls to TranslatePageImpl.
  base::WeakPtrFactory<TranslateHelper> weak_method_factory_;

  DISALLOW_COPY_AND_ASSIGN(TranslateHelper);
};

#endif  // CHROME_RENDERER_TRANSLATE_TRANSLATE_HELPER_H_
