(ns core.core
    (:require [reagent.core :as reagent :refer [atom]]
              [reagent.session :as session]
              [secretary.core :as secretary :include-macros true]
              [goog.events :as events]
              [goog.history.EventType :as EventType]
              [re-com.core :refer [v-box h-box box input-text gap]] ; border
              ;;[re-com.box    :refer [border-args-desc]]
              ;; NOTE: Include PouchDB in HTML
              )
    (:import goog.History))

;; -------------------------


;; -------------------------
;; Views

(def curr-user (reagent/atom ""))
(def search-val (reagent/atom ""))
(def todo-remove-me (reagent/atom ""))
(def should-hide-lhn (reagent/atom false))


(defn lhn-hide [_]
  (let [e (.getElementById js/document "lhndiv")
        v (.-display (.-style e))]
    (when (not (= v "none"))
      (set! (.-display (.-style e)) "none")))
  nil)

(defn lhn-show [_]
  (reset! should-hide-lhn false)
  (let [e (.getElementById js/document "lhndiv")
        v (.-display (.-style e))]
    (when (not (= v ""))
      (set! (.-display (.-style e)) "")))
  nil)

(defn lhn-delayed-hide [e]
  (reset! should-hide-lhn true)
  (js/setTimeout #(when @should-hide-lhn (lhn-hide e)) 100)
  nil)

(defn body-clicked [e]
  (lhn-delayed-hide e))

(defn lhn-prevent-hide [e]
  (reset! should-hide-lhn false)
  nil)

(defn hash-to-obj
  "Convert a CLJS structure to a JS object, yielding empty JS object
  for nil input"
  [obj]
  (or (clj->js obj) (js-obj)))

(defn get-db []
  (let [prom (js/PouchDB.
              "http://localhost:5984/auth"
              (js-obj :ajax {:cache false :timeout 10000}
                      :auth {:username ""
                             ;; TODO: Security: hard-coded password
                             :password ""}))]
    (if (not prom)
      (js/console.log "ERROR: Creating database. No promise returned.")
      (.catch
       (.then prom (fn [result]
                     (js/console.log
                      (str "INFO: Promise result: " result))
                     (.put result (hash-to-obj {:_id "doc002"
                                                :title "title002"}))))
       (fn [err] (js/console.log (str "ERROR: " err)))))))

(defn require-login []
  (when (empty? @curr-user)
    (let [db (get-db)])))

;; (js/PouchDB. "http://localhost:5984/auth")
(defn login-page []
  [:div [:h2 "Login"]
   [:div [:a {:href "#/"} "go to the home page"]]])

(defn home-page []
  (require-login)
  ;;; Begin Left-Hand Nav
  [:div {:id "maindiv"
         :style {:position "fixed" :height "100%" :width "100%"}}
   [:lhnav {:id "lhndiv" :style {:z-index 200 :display "none"}
            :on-click lhn-prevent-hide}
    [:div {:class "logo-area"}
     "Your logo here"]
    [:nav
     [:a {:href "http://www.bencluff.com"} "Home"]
     [:div {:class "nav-collapsible"}
      [:input {:type "checkbox" :id "nav-collapsible-1"}]
      [:label {:for "nav-collapsible-1"} "Portfolio"]
      [:div {:class "wrap"}
       [:a {:href "/"} "Art"]
       [:a {:href "/"} "Design"]
       [:a {:href "/"} "Print"]
       [:a {:href "/"} "Web"]]]
     [:div {:class "nav-collapsible"}
      [:input {:type "checkbox" :id "nav-collapsible-2"}]
      [:label {:for "nav-collapsible-2"} "Blog" ]
      [:div {:class "wrap"}
       [:a {:href "/"} "Art"]
       [:a {:href "/"} "Design"]
       [:a {:href "/"} "Print"]
       [:a {:href "/"} "Web"]]]
     [:a {:href "/"} "Contact"]]]
   ;;; End Left-Hand Nav

   [v-box :height "100%"
    :children
    [[box :child
      [:div {:style {:background-color "black" :position "fixed" :z-index 100
                     :opacity 0.5 :height "50px" :width "100%"}}
       [h-box
        :justify :center
        :style {:height "inherit"}
        :gap "16px"
        :children [[gap :size "0px"]
                   [box
                    :align-self :center
                    :child [:img {:id "menubtn1" :name "menubtn1"
                                  :on-click lhn-show
                                  :src "img/ic_menu_white_18dp.png"}]]
                   [gap :size "1"]
                   [h-box :gap "4px"
                    :children
                    [[box :align-self :center
                      :child [input-text :model search-val
                              :on-change #(reset! search-val %)
                              :width "200px"]]
                     [box :align-self :center
                      :child [:img {:id "searchimg"
                                    :src "img/ic_search_white_18dp.png"}]]]]
                   [gap :size "0px"]]]]]
     [box :align :stretch :size "auto"
      :attr {:id "asdf1"}
      :style {:background-color "khaki" :overflow-y "auto" :overflow-x "hidden"}
      :child
      [v-box :gap "15px"
       :style {:background-color "pink"}
       :children
       [[gap :size "80px"]
        [:span {:style {:position "relative" :left "20px"}}
         "Recently used apps"]
        [h-box
         :style {:background-color "lightblue"}
         :gap "15px"
         :children
         [[box :align-self :center :width "256px" :height "256px"
           :style {:border "1px dashed red" :border-radius "8px"}
           :child "App here"]
          [box :align-self :center :width "256px" :height "256px"
           :style {:border "1px dashed red" :border-radius "8px"}
           :child "App here"]
          [box :align-self :center :width "256px" :height "256px"
           :style {:border "1px dashed red" :border-radius "8px"}
           :child "App here"]
          [box :align-self :center :width "256px" :height "256px"
           :style {:border "1px dashed red" :border-radius "8px"}
           :child "App here"]
          [box :align-self :center :width "256px" :height "256px"
           :style {:border "1px dashed red" :border-radius "8px"}
           :child "App here"]
          [box :align-self :center :width "256px" :height "256px"
           :style {:border "1px dashed red" :border-radius "8px"}
           :child "App here"]
          [box :align-self :center :width "256px" :height "256px"
           :style {:border "1px dashed red" :border-radius "8px"}
           :child "App here"]
          [box :align-self :center :width "256px" :height "256px"
           :style {:border "1px dashed red" :border-radius "8px"}
           :child "App here"]]]]]]
     [:div {:style {:background-color "black" :position "fixed" :z-index 100
                    :opacity 0.5 :height "50px" :width "100%" :bottom "0px"}}
      [h-box
       :justify :center
       :style {:height "inherit"}
       :gap "96px"
       :children [[box :align-self :center
                   :child [:img {:src "img/ic_navigate_before_white_18dp.png"}]]
                  [box :align-self :center
                   :child [:img {:src "img/ic_home_white_18dp.png"}]]
                  [box :align-self :center
                   :child [:img {:src "img/ic_apps_white_18dp.png"}]]
                  ]]]]]])

(defn about-page []
  [:div [:h2 "About Core Apps"]
   [:div [:a {:href "#/"} "go to the home page"]]])

(defn current-page []
  [:div [(session/get :current-page)]])

;; -------------------------
;; Routes
(secretary/set-config! :prefix "#")

(secretary/defroute "/" []
  (session/put! :current-page #'home-page))

(secretary/defroute "/login" []
  (session/put! :current-page #'login-page))

(secretary/defroute "/about" []
  (session/put! :current-page #'about-page))

;; -------------------------
;; History
;; must be called after routes have been defined
(defn hook-browser-navigation! []
  (doto (History.)
    (events/listen
     EventType/NAVIGATE
     (fn [event]
       (secretary/dispatch! (.-token event))))
    (.setEnabled true)))

;; -------------------------
;; Initialize app
(defn mount-root []
  (reagent/render [current-page] (.getElementById js/document "app")))

(defn init! []
  (hook-browser-navigation!)
  (mount-root))
