/*
 * wiLink
 * Copyright (C) 2009-2015 Wifirst
 * See AUTHORS file for a full list of contributors.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

.pragma library

function datetimeFromString(text)
{
    function parseZeroInt(txt) {
        return parseInt(txt.replace(/^0/, ''));
    }

    var cap = text.match(/([0-9]{4})-([0-9]{2})-([0-9]{2})T([0-9]{2}):([0-9]{2}):([0-9]{2})Z/);
    if (cap) {
        var dt = new Date();
        dt.setUTCFullYear(parseInt(cap[1]));
        dt.setUTCMonth(parseZeroInt(cap[2]) - 1);
        dt.setUTCDate(parseZeroInt(cap[3]));
        dt.setUTCHours(parseZeroInt(cap[4]));
        dt.setUTCMinutes(parseZeroInt(cap[5]));
        dt.setUTCSeconds(parseZeroInt(cap[6]));
        return dt;
    }
}

function escapeRegExp(text) {
    return text.replace(/[-[\]{}()*+?.,\\^$|#\s]/g, '\\$&');
}

function formatDateTime(dateTime) {
    return Qt.formatDateTime(dateTime, 'dd MMM hh:mm');
}

function formatDuration(duration) {
    return duration + 's';
}

function formatNumber(size) {
    var KILO = 1000;
    var MEGA = KILO * 1000;
    var GIGA = MEGA * 1000;
    var TERA = GIGA * 1000;

    if (size < KILO)
        return size + ' ';
    else if (size < MEGA)
        return Math.floor(size / KILO) + ' K';
    else if (size < GIGA)
        return Math.floor(size / MEGA) + ' M';
    else if (size < TERA)
        return Math.floor(size / GIGA) + ' G';
    else
        return Math.floor(size / TERA) + ' T';
}

function formatSpeed(size) {
    return formatNumber(size * 8) + 'b/s';
}

function formatSize(size) {
    return formatNumber(size) + 'B';
}

function jidToBareJid(jid) {
    var pos = jid.indexOf('/');
    if (pos < 0)
        return jid;
    return jid.substring(0, pos);
}

function jidToDomain(jid) {
    var bareJid = jidToBareJid(jid);
    var pos = bareJid.indexOf('@');
    if (pos < 0)
        return bareJid;
    return bareJid.substring(pos + 1);
}

function jidToResource(jid) {
    var pos = jid.indexOf('/');
    if (pos < 0)
        return '';
    return jid.substring(pos + 1);
}

function jidToUser(jid) {
    var pos = jid.indexOf('@');
    if (pos < 0)
        return '';
    return jid.substring(0, pos);
}

// Checks whether the given object's properties match the given conditions.

function matchProperties(a, props) {
    if (props === undefined)
        return true;

    for (var key in props) {
        if (a[key] != props[key])
            return false;
    }
    return true;
}

// Returns a dump of the object's properties as a string.

function dumpProperties(a) {
    var dump = '';
    for (var key in a) {
        if (key != 'loadScript')
            dump += (dump.length > 0 ? ', ' : '') + key + ': ' + a[key];
    }
    return '{' + dump + '}';
}

// There's no getElementsByTagName method in QML
// ([http://www.mail-archive.com/qt-qml@trolltech.com/msg01024.html]), so we'll
// implement our own version.
function getElementsByTagName(rootElement, tagName) {
    var childNodes = rootElement.childNodes;
    var elements = [];
    for (var i = 0; i < childNodes.length; i++) {
        if (childNodes[i].nodeName == tagName) {
            elements.push(childNodes[i]);
        }
    }
    return elements;
}

function parseWilinkURI(uri) {
    // ex: wilink://verb?param1=value1&param2=value2
    var result = {verb: null, params: {}};
    if ( (uri == undefined) || (uri == null) || (uri.length == 0) )
        return result;
    var argv = decodeURIComponent(uri).split('wilink://');
    if (argv.length == 2) {
        argv = argv[1].split('?');

        // NOTE: on windows browsers seem to mangle the URL and add a slash
        result.verb = argv[0];
        if (result.verb[result.verb.length - 1] == '/') {
            result.verb = result.verb.substr(0, result.verb.length - 1);
        }

        if (argv.length >=2) {
            var params = argv[1].split('&');
            var keyval = [];
            for (var i=0 ; i < params.length ; i++) {
                keyval = params[i].split("=");
                if (keyval.length == 2) {
                    result.params[keyval[0]] = keyval[1];
                }
            }
        }
    }
    return result;
}

function iconText(iconStyle)
{
    switch (iconStyle) {
    case 'icon-glass':
        return '\uF000';
    case 'icon-music':
        return '\uF001';
    case 'icon-search':
        return '\uF002';
    case 'icon-envelope':
        return '\uF003';
    case 'icon-heart':
        return '\uF004';
    case 'icon-star':
        return '\uF005';
    case 'icon-star-empty':
        return '\uF006';
    case 'icon-user':
        return '\uF007';
    case 'icon-film':
        return '\uF008';
    case 'icon-th-large':
        return '\uF009';
    case 'icon-th':
        return '\uF00A';
    case 'icon-th-list':
        return '\uF00B';
    case 'icon-ok':
        return '\uF00C';
    case 'icon-remove':
        return '\uF00D';
    case 'icon-zoom-in':
        return '\uF00E';
    case 'icon-zoom-out':
        return '\uF010';
    case 'icon-off':
        return '\uF011';
    case 'icon-signal':
        return '\uF012';
    case 'icon-cog':
        return '\uF013';
    case 'icon-trash':
        return '\uF014';
    case 'icon-home':
        return '\uF015';
    case 'icon-file':
        return '\uF016';
    case 'icon-time':
        return '\uF017';
    case 'icon-road':
        return '\uF018';
    case 'icon-download-alt':
        return '\uF019';
    case 'icon-download':
        return '\uF01A';
    case 'icon-upload':
        return '\uF01B';
    case 'icon-inbox':
        return '\uF01C';
    case 'icon-play-circle':
        return '\uF01D';
    case 'icon-repeat':
        return '\uF01E';
    case 'icon-refresh':
        return '\uF021';
    case 'icon-list-alt':
        return '\uF022';
    case 'icon-lock':
        return '\uF023';
    case 'icon-flag':
        return '\uF024';
    case 'icon-headphones':
        return '\uF025';
    case 'icon-volume-off':
        return '\uF026';
    case 'icon-volume-down':
        return '\uF027';
    case 'icon-volume-up':
        return '\uF028';
    case 'icon-qrcode':
        return '\uF029';
    case 'icon-barcode':
        return '\uF02A';
    case 'icon-tag':
        return '\uF02B';
    case 'icon-tags':
        return '\uF02C';
    case 'icon-book':
        return '\uF02D';
    case 'icon-bookmark':
        return '\uF02E';
    case 'icon-print':
        return '\uF02F';
    case 'icon-camera':
        return '\uF030';
    case 'icon-font':
        return '\uF031';
    case 'icon-bold':
        return '\uF032';
    case 'icon-italic':
        return '\uF033';
    case 'icon-text-height':
        return '\uF034';
    case 'icon-text-width':
        return '\uF035';
    case 'icon-align-left':
        return '\uF036';
    case 'icon-align-center':
        return '\uF037';
    case 'icon-align-right':
        return '\uF038';
    case 'icon-align-justify':
        return '\uF039';
    case 'icon-list':
        return '\uF03A';
    case 'icon-indent-left':
        return '\uF03B';
    case 'icon-indent-right':
        return '\uF03C';
    case 'icon-facetime-video':
        return '\uF03D';
    case 'icon-picture':
        return '\uF03E';
    case 'icon-pencil':
        return '\uF040';
    case 'icon-map-marker':
        return '\uF041';
    case 'icon-adjust':
        return '\uF042';
    case 'icon-tint':
        return '\uF043';
    case 'icon-edit':
        return '\uF044';
    case 'icon-share':
        return '\uF045';
    case 'icon-check':
        return '\uF046';
    case 'icon-move':
        return '\uF047';
    case 'icon-step-backward':
        return '\uF048';
    case 'icon-fast-backward':
        return '\uF049';
    case 'icon-backward':
        return '\uF04A';
    case 'icon-play':
        return '\uF04B';
    case 'icon-pause':
        return '\uF04C';
    case 'icon-stop':
        return '\uF04D';
    case 'icon-forward':
        return '\uF04E';
    case 'icon-fast-forward':
        return '\uF050';
    case 'icon-step-forward':
        return '\uF051';
    case 'icon-eject':
        return '\uF052';
    case 'icon-chevron-left':
        return '\uF053';
    case 'icon-chevron-right':
        return '\uF054';
    case 'icon-plus-sign':
        return '\uF055';
    case 'icon-minus-sign':
        return '\uF056';
    case 'icon-remove-sign':
        return '\uF057';
    case 'icon-ok-sign':
        return '\uF058';
    case 'icon-question-sign':
        return '\uF059';
    case 'icon-info-sign':
        return '\uF05A';
    case 'icon-screenshot':
        return '\uF05B';
    case 'icon-remove-circle':
        return '\uF05C';
    case 'icon-ok-circle':
        return '\uF05D';
    case 'icon-ban-circle':
        return '\uF05E';
    case 'icon-arrow-left':
        return '\uF060';
    case 'icon-arrow-right':
        return '\uF061';
    case 'icon-arrow-up':
        return '\uF062';
    case 'icon-arrow-down':
        return '\uF063';
    case 'icon-share-alt':
        return '\uF064';
    case 'icon-resize-full':
        return '\uF065';
    case 'icon-resize-small':
        return '\uF066';
    case 'icon-plus':
        return '\uF067';
    case 'icon-minus':
        return '\uF068';
    case 'icon-asterisk':
        return '\uF069';
    case 'icon-exclamation-sign':
        return '\uF06A';
    case 'icon-gift':
        return '\uF06B';
    case 'icon-leaf':
        return '\uF06C';
    case 'icon-fire':
        return '\uF06D';
    case 'icon-eye-open':
        return '\uF06E';
    case 'icon-eye-close':
        return '\uF070';
    case 'icon-warning-sign':
        return '\uF071';
    case 'icon-plane':
        return '\uF072';
    case 'icon-calendar':
        return '\uF073';
    case 'icon-random':
        return '\uF074';
    case 'icon-comment':
        return '\uF075';
    case 'icon-magnet':
        return '\uF076';
    case 'icon-chevron-up':
        return '\uF077';
    case 'icon-chevron-down':
        return '\uF078';
    case 'icon-retweet':
        return '\uF079';
    case 'icon-shopping-cart':
        return '\uF07A';
    case 'icon-folder-close':
        return '\uF07B';
    case 'icon-folder-open':
        return '\uF07C';
    case 'icon-resize-vertical':
        return '\uF07D';
    case 'icon-resize-horizontal':
        return '\uF07E';
    case 'icon-bar-chart':
        return '\uF080';
    case 'icon-twitter-sign':
        return '\uF081';
    case 'icon-facebook-sign':
        return '\uF082';
    case 'icon-camera-retro':
        return '\uF083';
    case 'icon-key':
        return '\uF084';
    case 'icon-cogs':
        return '\uF085';
    case 'icon-comments':
        return '\uF086';
    case 'icon-thumbs-up':
        return '\uF087';
    case 'icon-thumbs-down':
        return '\uF088';
    case 'icon-star-half':
        return '\uF089';
    case 'icon-heart-empty':
        return '\uF08A';
    case 'icon-signout':
        return '\uF08B';
    case 'icon-linkedin-sign':
        return '\uF08C';
    case 'icon-pushpin':
        return '\uF08D';
    case 'icon-external-link':
        return '\uF08E';
    case 'icon-signin':
        return '\uF090';
    case 'icon-trophy':
        return '\uF091';
    case 'icon-github-sign':
        return '\uF092';
    case 'icon-upload-alt':
        return '\uF093';
    case 'icon-lemon':
        return '\uF094';
    case 'icon-phone':
        return '\uF095';
    case 'icon-check-empty':
        return '\uF096';
    case 'icon-bookmark-empty':
        return '\uF097';
    case 'icon-phone-sign':
        return '\uF098';
    case 'icon-twitter':
        return '\uF099';
    case 'icon-facebook':
        return '\uF09A';
    case 'icon-github':
        return '\uF09B';
    case 'icon-unlock':
        return '\uF09C';
    case 'icon-credit-card':
        return '\uF09D';
    case 'icon-rss':
        return '\uF09E';
    case 'icon-hdd':
        return '\uF0A0';
    case 'icon-bullhorn':
        return '\uF0A1';
    case 'icon-bell':
        return '\uF0A2';
    case 'icon-certificate':
        return '\uF0A3';
    case 'icon-hand-right':
        return '\uF0A4';
    case 'icon-hand-left':
        return '\uF0A5';
    case 'icon-hand-up':
        return '\uF0A6';
    case 'icon-hand-down':
        return '\uF0A7';
    case 'icon-circle-arrow-left':
        return '\uF0A8';
    case 'icon-circle-arrow-right':
        return '\uF0A9';
    case 'icon-circle-arrow-up':
        return '\uF0AA';
    case 'icon-circle-arrow-down':
        return '\uF0AB';
    case 'icon-globe':
        return '\uF0AC';
    case 'icon-wrench':
        return '\uF0AD';
    case 'icon-tasks':
        return '\uF0AE';
    case 'icon-filter':
        return '\uF0B0';
    case 'icon-briefcase':
        return '\uF0B1';
    case 'icon-fullscreen':
        return '\uF0B2';
    case 'icon-group':
        return '\uF0C0';
    case 'icon-link':
        return '\uF0C1';
    case 'icon-cloud':
        return '\uF0C2';
    case 'icon-beaker':
        return '\uF0C3';
    case 'icon-cut':
        return '\uF0C4';
    case 'icon-copy':
        return '\uF0C5';
    case 'icon-paper-clip':
        return '\uF0C6';
    case 'icon-save':
        return '\uF0C7';
    case 'icon-sign-blank':
        return '\uF0C8';
    case 'icon-reorder':
        return '\uF0C9';
    case 'icon-list-ul':
        return '\uF0CA';
    case 'icon-list-ol':
        return '\uF0CB';
    case 'icon-strikethrough':
        return '\uF0CC';
    case 'icon-underline':
        return '\uF0CD';
    case 'icon-table':
        return '\uF0CE';
    case 'icon-magic':
        return '\uF0D0';
    case 'icon-truck':
        return '\uF0D1';
    case 'icon-pinterest':
        return '\uF0D2';
    case 'icon-pinterest-sign':
        return '\uF0D3';
    case 'icon-google-plus-sign':
        return '\uF0D4';
    case 'icon-google-plus':
        return '\uF0D5';
    case 'icon-money':
        return '\uF0D6';
    case 'icon-caret-down':
        return '\uF0D7';
    case 'icon-caret-up':
        return '\uF0D8';
    case 'icon-caret-left':
        return '\uF0D9';
    case 'icon-caret-right':
        return '\uF0DA';
    case 'icon-columns':
        return '\uF0DB';
    case 'icon-sort':
        return '\uF0DC';
    case 'icon-sort-down':
        return '\uF0DD';
    case 'icon-sort-up':
        return '\uF0DE';
    case 'icon-envelope-alt':
        return '\uF0E0';
    case 'icon-linkedin':
        return '\uF0E1';
    case 'icon-undo':
        return '\uF0E2';
    case 'icon-legal':
        return '\uF0E3';
    case 'icon-dashboard':
        return '\uF0E4';
    case 'icon-comment-alt':
        return '\uF0E5';
    case 'icon-comments-alt':
        return '\uF0E6';
    case 'icon-bolt':
        return '\uF0E7';
    case 'icon-sitemap':
        return '\uF0E8';
    case 'icon-umbrella':
        return '\uF0E9';
    case 'icon-paste':
        return '\uF0EA';
    case 'icon-user-md':
        return '\uF200';
    default:
        return '';
    }
}
