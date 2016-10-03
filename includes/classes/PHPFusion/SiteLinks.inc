<?php
/*-------------------------------------------------------+
| PHP-Fusion Content Management System
| Copyright (C) PHP-Fusion Inc
| https://www.php-fusion.co.uk/
+--------------------------------------------------------+
| Filename: SiteLinks.inc
| Author: Frederick MC Chan (Chan)
+--------------------------------------------------------+
| This program is released as free software under the
| Affero GPL license. You can redistribute it and/or
| modify it under the terms of this license which you
| can read by viewing the included agpl.txt or online
| at www.gnu.org/licenses/agpl.html. Removal of this
| copyright header is strictly prohibited without
| written permission from the original author(s).
+--------------------------------------------------------*/
namespace PHPFusion;

class SiteLinks {

    protected static $position_opts = array();
    private static $instances = [];
    private static $menuOptions = [];
    private static $primary_cache_data = [];
    private static $optional_cache_data = [];

    /**
     * Get Site Links Position Options
     * @return array
     */
    public static function get_SiteLinksPosition() {
        $locale = fusion_get_locale('', LOCALE.LOCALESET."admin/sitelinks.php");
        if (empty(self::$position_opts)) {
            self::$position_opts = array(
                '1' => $locale['SL_0025'], // only css navigational panel
                '2' => $locale['SL_0026'], // both
                '3' => $locale['SL_0027'], // subheader
                '4' => $locale['custom']." ID",
            );
        }

        return (array)self::$position_opts;
    }

    /**
     * Get Sitelinks SQL Row
     * @param $id
     * @return array|bool
     */
    public static function get_SiteLinks($id) {
        $data = array();
        $link_query = "SELECT * FROM ".DB_SITE_LINKS." ".(multilang_table("SL") ? "WHERE link_language='".LANGUAGE."' AND" : "WHERE")." link_id='$id'";
        $result = dbquery($link_query);
        if (dbrows($result) > 0) {
            $data = dbarray($result);
        }

        return $data;
    }

    /**
     * Given a matching URL, fetch Sitelinks data
     * @param string $url - url to match (link_url) column
     * @param string $key - column data to output, blank for all
     * @return array|bool
     */
    public static function get_current_SiteLinks($url = "", $key = NULL) {
        $url = stripinput($url);
        static $data = array();
        if (empty($data)) {
            if (!$url) {
                $pathinfo = pathinfo($_SERVER['PHP_SELF']);
                $url = FUSION_FILELINK;
            }
            $result = dbquery("SELECT * FROM ".DB_SITE_LINKS." WHERE link_url='".$url."' AND link_language='".LANGUAGE."'");
            if (dbrows($result) > 0) {
                $data = dbarray($result);
            }
        }

        return $key === NULL ? (array)$data : (isset($data[$key]) ? $data[$key] : NULL);
    }

    /**
     * Link ID validation
     * @param $link_id
     * @return bool|string
     */
    public static function verify_sitelinks($link_id) {
        if (isnum($link_id)) {
            return dbcount("(link_id)", DB_SITE_LINKS, "link_id='".intval($link_id)."'");
        }

        return FALSE;
    }

    /**
     * SQL Delete Site Link Action
     * @param $link_id
     * @return bool|mixed|null|PDOStatement|resource
     */
    public static function delete_sitelinks($link_id) {
        $result = NULL;
        if (isnum($link_id)) {
            $data = dbarray(dbquery("SELECT link_order FROM ".DB_SITE_LINKS." ".(multilang_table("SL") ? "WHERE link_language='".LANGUAGE."' AND" : "WHERE")." link_id='".$_GET['link_id']."'"));
            $result = dbquery("UPDATE ".DB_SITE_LINKS." SET link_order=link_order-1 ".(multilang_table("SL") ? "WHERE link_language='".LANGUAGE."' AND" : "WHERE")." link_order>'".$data['link_order']."'");
            if ($result) {
                $result = dbquery("DELETE FROM ".DB_SITE_LINKS." WHERE link_id='".$_GET['link_id']."'");
            }

            return $result;
        }

        return $result;
    }

    /**
     * Get Group Array
     * @return array
     */
    public static function get_LinkVisibility() {
        static $visibility_opts = array();
        $user_groups = getusergroups();
        while (list($key, $user_group) = each($user_groups)) {
            $visibility_opts[$user_group['0']] = $user_group['1'];
        }

        return (array)$visibility_opts;
    }

    /**
     * @param string $sep
     * @param string $class
     * @param array  $options
     * @return static
     *
     * A blank static is set up once for each available $options['id']
     * If same instance exists, options can be mutated to alter the behavior of the menu
     *
     * Simple Usage: SiteLinks::setSublinks($sep, $class, $options)->showSubLinks();
     *
     * So in order to add a cart icon, we must declare at theme.
     *
     */

    public static function setSubLinks($sep = "", $class = "", array $options = array()) {
        /*
         * If set an ID, it will re-run the class to create a new object again.
         */
        $options += [// $default_options = [
                     'id' => '',
                     'container' => FALSE,
                     'navbar_class' => 'navbar-default',
                     'item_class' => '',
                     'locale' => [],
                     'separator' => '',
                     'callback_data' => [],
                     'additional_data' => [],
                     'links_per_page' => '',
                     'grouping' => '',
                     'show_banner' => FALSE,
                     'show_header' => FALSE,
                     'language_switcher' => FALSE,
                     'searchbar' => FALSE,
        ];

        if (!isset(self::$instances[$options['id']])) {

            $options['locale'] += fusion_get_locale();

            if (empty($options['seperator'])) {
                $options['seperator'] = $sep;
            }

            if (empty($options['item_class'])) {
                $options['item_class'] = $class;
            }

            if (!$options['links_per_page']) {
                $options['links_per_page'] = fusion_get_settings('links_per_page');
            }

            if (empty($options['grouping'])) {
                $options['grouping'] = fusion_get_settings('links_grouping');
            }

            // This method need to deprecate in order for menu param to work flawlessly
            if (empty($options['id'])) {
                $options['id'] = md5(str_shuffle(str_replace(" ", "_", fusion_get_settings("sitename"))));
            }

            if (empty($options['callback_data'])) {
                $options['callback_data'] = self::get_SiteLinksData(array('link_position' => [2, 3]));
            }

            /**
             * Change hierarchy data when grouping is on
             */
            if ($options['grouping'] == TRUE) {
                if (count($options['callback_data'][0]) > $options['links_per_page']) {
                    $more_index = 9 * 10000000;
                    $base_data = $options['callback_data'][0];
                    $data[$more_index] = array_slice($base_data, $options['links_per_page'], 9, TRUE);
                    $data[0] = array_slice($base_data, 0, $options['links_per_page'], TRUE);
                    $more[$more_index] = array(
                        "link_id" => $more_index,
                        "link_cat" => 0,
                        "link_name" => $options['locale']['global_700'],
                        "link_url" => "#",
                        "link_icon" => "",
                        "link_visibility" => 0,
                        "link_position" => 2,
                        "link_window" => 0,
                        "link_order" => $options['links_per_page'],
                        "link_language" => LANGUAGE
                    );
                    $data[0] += $more;
                }
            }

            $options['banner'] = fusion_get_settings('sitebanner') && $options['show_banner'] == TRUE ? "<img src='".BASEDIR.fusion_get_settings("sitebanner")."' alt='".fusion_get_settings("sitename")."'/>" : fusion_get_settings("sitename");

            $pageInfo = pathinfo($_SERVER['REQUEST_URI']);
            $start_page = $pageInfo['dirname'] !== "/" ? ltrim($pageInfo['dirname'], "/")."/" : "";
            $site_path = ltrim(fusion_get_settings("site_path"), "/");
            $start_page = str_replace($site_path, "", $start_page);
            $start_page .= $pageInfo['basename'];

            if (fusion_get_settings("site_seo") && defined('IN_PERMALINK') && !isset($_GET['aid'])) {
                global $filepath;
                $start_page = $filepath;
            }

            $options['start_page'] = $start_page;

            self::$instances[$options['id']] = new static();

            self::$menuOptions = $options;

        }

        // Fetches custom hardcoded links
        self::$menuOptions['callback_data'] = array_merge_recursive(self::$menuOptions['callback_data'], self::$primary_cache_data);

        self::$menuOptions['additional_data'] = array_merge_recursive(self::$menuOptions['additional_data'], self::$optional_cache_data);

        return self::$instances[$options['id']];
    }

    /**
     * Fetches Site Links Hierarchy Data - for a less support complexity
     * @param array $options
     * - join
     * - link_position (array)
     * - condition
     * - group
     * - order
     * @return array
     */
    public static function get_SiteLinksData(array $options = array()) {

        $default_position = array(2, 3);

        $default_link_filter = array(
            'join' => '',
            'position_condition' => '(sl.link_position='.($options['link_position'] ? implode(' OR sl.link_position=',
                                                                                              $options['link_position']) : implode(' OR sl.link_position=',
                                                                                                                                   $default_position)).')',
            'condition' => (multilang_table("SL") ? " AND link_language='".LANGUAGE."'" : "")." AND ".groupaccess('link_visibility'),
            'group' => '',
            'order' => "link_cat ASC, link_order ASC",
        );
        $options += $default_link_filter;

        $query_replace = "";
        if (!empty($options)) {
            $query_replace = "SELECT sl.* ".(!empty($options['select']) ? ", ".$options['select'] : '')." ";
            $query_replace .= "FROM ".DB_SITE_LINKS." sl ";
            $query_replace .= $options['join']." ";
            $query_replace .= "WHERE ".$options['position_condition'].$options['condition'];
            $query_replace .= (!empty($options['group']) ? " GROUP BY ".$options['group']." " : "")." ORDER BY ".$options['order'];
        }

        return (array)dbquery_tree_full(DB_SITE_LINKS, "link_id", "link_cat", "", $query_replace);
    }

    /**
     * Add a link to primary menu
     * @param            int /string $link_id
     * @param string     $link_name
     * @param int        $link_cat
     * @param string     $link_url
     * @param string     $link_icon
     * @param bool|FALSE $link_active
     * @param bool|FALSE $link_title
     * @param bool|FALSE $link_disabled
     * @param bool|FALSE $link_window
     */
    public static function addMenuLink($link_id = 0, $link_name, $link_cat = 0, $link_url = '', $link_icon = '', $link_active = FALSE, $link_title = FALSE, $link_disabled = FALSE, $link_window = FALSE) {
        self::$primary_cache_data[$link_cat][$link_id] = [
            'link_id' => $link_id,
            'link_name' => $link_name,
            'link_url' => $link_url,
            'link_icon' => $link_icon,
            'link_active' => $link_active,
            'link_title' => $link_title,
            'link_disabled' => $link_disabled,
            'link_window' => $link_window
        ];
    }

    /**
     * Add a link to secondary menu
     * @param            int /string $link_id
     * @param string     $link_name
     * @param int        $link_cat
     * @param string     $link_url
     * @param string     $link_icon
     * @param bool|FALSE $link_active
     * @param bool|FALSE $link_title
     * @param bool|FALSE $link_disabled
     * @param bool|FALSE $link_window
     */
    public static function addOptionalMenuLink($link_id = 0, $link_name, $link_cat = 0, $link_url = '', $link_icon = '', $link_active = FALSE, $link_title = FALSE, $link_disabled = FALSE, $link_window = FALSE) {
        self::$optional_cache_data[$link_cat][$link_id] = [
            'link_id' => $link_id,
            'link_name' => $link_name,
            'link_url' => $link_url,
            'link_icon' => $link_icon,
            'link_active' => $link_active,
            'link_title' => $link_title,
            'link_disabled' => $link_disabled,
            'link_window' => $link_window
        ];
    }

    public function setMenuParam($key, $value) {
        self::$MenuOptions[$key] = (is_bool($value)) ? $value : self::getMenuParam($key).$value;
    }

    protected static function getMenuParam($key = FALSE) {
        if (isset(self::$menuOptions[$key])) { // will return an error if $prop is not available
            return self::$menuOptions[$key];
        } else {
            print_p($key);
            debug_print_backtrace();
        }

        return NULL;
    }

    public function showSubLinks($id = 0) {
        if (empty($id)) {
            $res = "<div id='".self::$menuOptions['id']."' class='navbar ".self::$menuOptions['navbar_class']."' role='navigation'>\n";
            $res .= self::$menuOptions['container'] ? "<div class='container'>\n" : "";
            $res .= "<div class='navbar-header'>\n";
            $res .= "<!---Menu Header Start--->\n";
            $res .= "<button type='button' class='navbar-toggle collapsed' data-toggle='collapse' data-target='#".self::$menuOptions['id']."_menu' aria-expanded='false'>\n";
            $res .= "<span class='sr-only'>".self::$menuOptions['locale']['global_017']."</span>\n";
            $res .= "<span class='icon-bar top-bar'></span>\n";
            $res .= "<span class='icon-bar middle-bar'></span>\n";
            $res .= "<span class='icon-bar bottom-bar'></span>\n";
            $res .= "</button>\n";
            if (self::$menuOptions['show_header']) {
                if (self::$menuOptions['show_header'] === TRUE) {
                    $res .= "<a class='navbar-brand ".fusion_get_settings('logoposition_xs')." ".fusion_get_settings('logoposition_sm')." ".fusion_get_settings('logoposition_md')." ".fusion_get_settings('logoposition_lg')."' href='".BASEDIR.fusion_get_settings('opening_page')."'>".self::$menuOptions['banner']."</a>\n";
                } else {
                    $res .= self::$menuOptions['show_header'];
                }
            } else {
                $res .= "<a class='navbar-brand visible-xs hidden-sm hidden-md hidden-lg' href='".BASEDIR.fusion_get_settings('opening_page')."'>".fusion_get_settings("sitename")."</a>\n";
            }
            $res .= "<!---Menu Header End--->\n";
            $res .= "</div>\n";
            $res .= "<div class='navbar-collapse collapse' id='".self::$menuOptions['id']."_menu'>\n";
            $res .= "<ul ".(fusion_get_settings("bootstrap") ? "class='nav navbar-nav primary'" : "id='main-menu' class='primary sm sm-simple'").">\n";
            $res .= "<!---Menu Item Start--->\n";
        }

        $res .= $this->showMenuLinks($id, self::$menuOptions['callback_data']);

        if (empty($id)) {
            $res .= "<!---Menu Item End--->\n";
            $res .= "</ul>\n";

            if (self::$menuOptions['language_switcher'] == TRUE || self::$menuOptions['searchbar'] == TRUE || !empty(self::$menuOptions['additional_data'])) {
                $res .= "<ul class='nav navbar-nav navbar-right'>\n";

                $res .= $this->showMenuLinks($id, self::$menuOptions['additional_data']);

                if (self::$menuOptions['language_switcher'] == TRUE) {
                    if (count(fusion_get_enabled_languages()) > 1) {
                        $language_switch = fusion_get_language_switch();
                        $current_language = $language_switch[LANGUAGE];
                        $language_opts = "<li class='dropdown'>";
                        $language_opts .= "<a class='dropdown-toggle pointer' data-toggle='dropdown' title='".translate_lang_names(LANGUAGE)."'><img src='".$current_language['language_icon_s']."'/> <span class='caret'></span></a>";
                        $language_opts .= "<ul class='dropdown-menu' role='menu'>\n";
                        if (!empty($language_switch)) {
                            foreach ($language_switch as $folder => $langData) {
                                $language_opts .= "<li class='text-left'><a href='".$langData['language_link']."'>";
                                $language_opts .= "<img alt='".$langData['language_name']."' class='m-r-5' src='".$langData['language_icon_s']."'/>";
                                $language_opts .= $langData['language_name'];
                                $language_opts .= "</a></li>\n";
                            }
                        }
                        $language_opts .= "</ul>\n";
                        $language_opts .= "</li>\n";
                        $res .= $language_opts;
                    }
                }

                if (self::$menuOptions['searchbar'] == TRUE) {
                    $searchbar = "<li class='dropdown'>";
                    $searchbar .= "<a class='dropdown-toggle pointer' data-toggle='dropdown' title='".fusion_get_locale('search')."'><i class='fa fa-search fa-fw'></i></a>";
                    $searchbar .= "<ul class='dropdown-menu p-l-15 p-r-15 p-t-15' role='menu' style='min-width: 300px;'>\n";
                    $searchbar .= "<li class='text-left'>";
                    $searchbar .= openform('searchform', 'post', BASEDIR.'search.php?stype=all',
                                           array(
                                               'class' => 'm-b-10',
                                               'remote_url' => fusion_get_settings('site_path')."search.php"
                                           )
                    );
                    $searchbar .= form_text('stext', '', '',
                                            array(
                                                'placeholder' => self::$menuOptions['locale']['search'],
                                                'append_button' => TRUE,
                                                'append_type' => "submit",
                                                "append_form_value" => self::$menuOptions['locale']['search'],
                                                "append_value" => "<i class='fa fa-search'></i> ".self::$menuOptions['locale']['search'],
                                                "append_button_name" => "search",
                                                'class' => 'no-border m-0'
                                            )
                    );
                    $searchbar .= closeform();
                    $searchbar .= "</li>\n";
                    $res .= $searchbar;
                }

                $res .= "</ul>\n";
            }
            $res .= self::$menuOptions['container'] ? "</div>\n" : "";
            $res .= "</div>\n</div>\n";
        }

        return $res;
    }

    /*
     * Recursion loop of data
     */

    private function showMenuLinks($id, $data) {

        $res = &$res;

        if (!empty($data)) {
            $i = 0;

            $default_link_data = array(
                "link_id" => 0,
                "link_name" => "",
                "link_cat" => 0,
                "link_url" => "",
                "link_icon" => "",
                "link_active" => FALSE,
                "link_title" => FALSE, // true to add dropdown-header class to li.
                "link_disabled" => FALSE, // true to disable link
                "link_window" => FALSE,
            );

            foreach ($data[$id] as $link_id => $link_data) {
                $li_class = [];
                $link_data += $default_link_data;

                $link_data['link_name'] = parsesmileys(parseubb($link_data['link_name']));

                $li_class[] = self::$menuOptions['item_class'];
                if (empty($link_data['link_url'])) {
                    $li_class[] = "no-link";
                }

                if ($link_data['link_disabled']) {
                    $li_class[] = "disabled";
                } else {
                    if ($link_data['link_title'] == TRUE) {
                        $li_class[] = "dropdown-header";
                    }
                }

                /*
                 * Attempt to calculate a relative link
                 * Taking into account that current start page does not match
                 */
                $secondary_active = FALSE;
                if (self::$menuOptions['start_page'] !== $link_data['link_url']) {
                    $link_instances = BreadCrumbs::getInstance();
                    $link_instances->showHome(FALSE);
                    $reference = $link_instances->toArray();
                    if (!empty($reference)) {
                        foreach ($reference as $refData) {
                            if (!empty($link_data['link_url']) && !empty($refData['link']) && $link_data['link_url'] !== "index.php") {
                                if (stristr($refData['link'], str_replace("index.php", "", $link_data['link_url']))) {
                                    $secondary_active = TRUE;
                                }
                                break; // match found
                            }
                        }
                    }
                }

                if ($link_data['link_name'] != "---" && $link_data['link_name'] != "===") {

                    $link_target = ($link_data['link_window'] == "1" ? " target='_blank'" : '');
                    if ($i == 0 && $id > 0) {
                        $li_class[] = "first-link";
                    }

                    $link_is_active = FALSE;
                    if ($secondary_active) {
                        $link_is_active = TRUE;
                    } elseif (self::$menuOptions['start_page'] == $link_data['link_url']) {
                        $link_is_active = TRUE;
                    } elseif (fusion_get_settings('site_path').self::$menuOptions['start_page'] == $link_data['link_url']) {
                        $link_is_active = TRUE;
                    } elseif ((self::$menuOptions['start_page'] == fusion_get_settings("opening_page") && $i == 0 && $id === 0)) {
                        $link_is_active = TRUE;
                    }
                    if ($link_data['link_url'] === '#') {
                        $link_is_active = FALSE;
                    }
                    if ($link_is_active) {
                        $li_class[] = "current-link active";
                    }

                    $itemlink = '';
                    if (!empty($link_data['link_url'])) {
                        $itemlink = " href='".BASEDIR.$link_data['link_url']."' ";
                        // if link has site protocol
                        if (preg_match("!^(ht|f)tp(s)?://!i", $link_data['link_url'])
                            or (BASEDIR !== '' && stristr($link_data['link_url'], BASEDIR))
                        ) {
                            $itemlink = " href='".$link_data['link_url']."' ";
                        }
                    }

                    $has_child = FALSE;
                    $l_1 = "";
                    $l_2 = "";
                    $tab_index = "";

                    if (isset($data[$link_id])) {
                        $has_child = TRUE;
                        $l_1 = " class='dropdown-toggle' data-toggle='dropdown' ";
                        $l_1 .= (empty($id) && $has_child ? "data-submenu " : "");
                        $l_2 = (empty($id) ? "<i class='caret'></i>" : "");
                        $li_class[] = (!empty($id) ? "dropdown-submenu" : "dropdown");
                        $tab_index .= !empty($id) ? "tabindex='0'" : "";
                    }

                    $li_class = array_filter($li_class);
                    $res .= "<li".(!empty($li_class) ? " class='".implode(" ", $li_class)."'" : '').">".self::$menuOptions['separator']."";
                    $res .= ($itemlink ? "<a".$l_1.$itemlink.$link_target." $tab_index>" : "");
                    $res .= (!empty($link_data['link_icon']) ? "<i class='".$link_data['link_icon']."'></i>" : "");
                    $res .= $link_data['link_name'].$l_2;
                    $res .= ($itemlink ? "</a>" : '');

                    if ($has_child) {
                        $res .= "\n<ul".(fusion_get_settings("bootstrap") ? " class='dropdown-menu'" : "").">\n";
                        if (!empty($link_data['link_url']) and $link_data['link_url'] !== "#") {
                            $res .= "<li".(!$itemlink ? " class='no-link'" : '').">".self::$menuOptions['separator']."";
                            $res .= ($itemlink ? "<a href='".$itemlink."'".$link_target.">" : '');
                            $res .= (!empty($link_data['link_icon']) ? "<i class='".$link_data['link_icon']."'></i>" : "");
                            $res .= $link_data['link_name'];
                            $res .= ($itemlink ? "</a>" : '');
                            $res .= "</li>\n";
                        }

                        $res .= $this->showMenuLinks($link_data['link_id'], $data);
                        $res .= "</ul>\n";
                    }
                    $res .= "</li>\n";
                } elseif ($link_data['link_cat'] > 0) {
                    $res .= "<li class='divider'></li>\n";
                }
                $i++;
            }
        }

        return $res;
    }

}