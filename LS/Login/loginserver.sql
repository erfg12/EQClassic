DROP TABLE IF EXISTS `login_accounts`;
CREATE TABLE IF NOT EXISTS `login_accounts` (
  `id` int(11) unsigned NOT NULL auto_increment,
  `name` varchar(18) NOT NULL default '',
  `password` varchar(45) NOT NULL,
  `ip` varchar(16) NOT NULL default '',
  `auth` tinyblob,
  `lsadmin` tinyint(2) unsigned NOT NULL default '0',
  `lsstatus` tinyint(2) unsigned NOT NULL default '0',
  `worldadmin` varchar(45) NOT NULL,
  `user_active` varchar(45) NOT NULL,
  `user_lastvisit` varchar(45) NOT NULL,
  PRIMARY KEY  (`id`),
  UNIQUE KEY `name` (`name`)
) ENGINE=MyISAM  DEFAULT CHARSET=latin1 AUTO_INCREMENT=2 ;

CREATE TABLE IF NOT EXISTS `login_authchange` (
  `account_id` int(11) unsigned NOT NULL default '0',
  `ip` varchar(16) NOT NULL default '',
  `time` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  PRIMARY KEY  (`account_id`),
  UNIQUE KEY `ip` (`ip`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

CREATE TABLE IF NOT EXISTS `login_versions` (
  `approval` int(10) unsigned NOT NULL,
  `version` varchar(45) NOT NULL,
  PRIMARY KEY  (`approval`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE IF NOT EXISTS `login_worldservers` (
  `id` int(11) unsigned NOT NULL auto_increment,
  `account` varchar(30) NOT NULL default '',
  `password` varchar(30) NOT NULL default '',
  `name` varchar(250) NOT NULL default '',
  `admin_id` int(11) unsigned NOT NULL default '0',
  `greenname` tinyint(1) unsigned NOT NULL default '0',
  `showdown` tinyint(4) NOT NULL default '0',
  `chat` tinyint(4) NOT NULL default '0',
  `note` tinytext,
  PRIMARY KEY  (`id`),
  UNIQUE KEY `account` (`account`)
) ENGINE=MyISAM  DEFAULT CHARSET=latin1 AUTO_INCREMENT=2 ;