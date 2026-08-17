import os
import shutil

def on_post_build(config):
    site_dir = config['site_dir']
    root_sitemap = os.path.join(site_dir, 'sitemap.xml')
    pt_dir = os.path.join(site_dir, 'pt')
    if os.path.exists(root_sitemap) and os.path.exists(pt_dir):
        shutil.copyfile(root_sitemap, os.path.join(pt_dir, 'sitemap.xml'))
        root_gz = os.path.join(site_dir, 'sitemap.xml.gz')
        if os.path.exists(root_gz):
            shutil.copyfile(root_gz, os.path.join(pt_dir, 'sitemap.xml.gz'))
