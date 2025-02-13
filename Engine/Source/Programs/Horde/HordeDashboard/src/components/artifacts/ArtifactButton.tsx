import { IContextualMenuProps, DirectionalHint, PrimaryButton } from "@fluentui/react";
import backend from "horde/backend";
import { ArtifactBrowserItem, ArtifactBrowserType, ArtifactSelection, CreateZipRequest, DownloadFormat, GetArtifactResponse } from "horde/backend/Api";
import dashboard from "horde/backend/Dashboard";
import { formatBytes } from "horde/base/utilities/stringUtills";
import { useNavigate } from "react-router-dom";


export const ArtifactButton: React.FC<{ jobId?: string, stepId?: string, artifact?: GetArtifactResponse, pathIn?: string, selection?: ArtifactSelection, openArtifactInfo?: () => void, disabled?: boolean, minWidth?: number }> = ({ jobId, stepId, artifact, pathIn, selection, openArtifactInfo, disabled, minWidth }) => {

   const navigate = useNavigate();

   const format = dashboard.downloadFormat;
   const path = pathIn ? pathIn + "/" : "";
   const items = selection?.items;

   let sizeText = "0KB";
   if (selection?.size) {
      sizeText = formatBytes(selection.size, (selection.size < (1024 * 1024)) ? 0 : 1)
   }

   let buttonText = "Download";

   if (selection?.directoriesSelected || (selection?.filesSelected ?? 0) > 1) {
      buttonText = `Download (${sizeText})`;
   } else if (selection?.filesSelected === 1) {
      buttonText = `Download (${sizeText})`;
   }

   let jobUrl = "";
   if (jobId) {
      jobUrl = `/job/${jobId}`;
      if (stepId) {
         jobUrl += `?step=${stepId}`;
      }
   }

   const downloadProps: IContextualMenuProps = {
      items: [],
      directionalHint: DirectionalHint.bottomRightEdge
   };

   if (openArtifactInfo) {
      downloadProps.items.push({
         key: 'view_artifact_info',
         text: 'View Artifact Info',
         disabled: !artifact,
         onClick: () => {
            openArtifactInfo();
         }
      })
   }

   if (jobUrl) {
      {
         downloadProps.items.push({
            key: 'navigate_to_job',
            text: 'Navigate to Job',
            onClick: () => {
               navigate(jobUrl);
            }
         })
      }
   }

   function downloadToolbox() {
      if (artifact?.id) {
         window.location.assign(`horde-artifact://${window.location.hostname}:${window.location.port}/api/v2/artifacts/${artifact.id}`);
      }
   }

   function downloadUGS() {
      if (artifact?.id) {
         window.location.assign(`/api/v2/artifacts/${artifact.id}/download?format=ugs`);
      }
   }

   function downloadZip() {

      if (!artifact) {
         return;
      }

      if (!items?.length) {
         window.location.assign(`/api/v2/artifacts/${artifact.id}/download?format=zip`);
         return;
      }

      // download a single file
      if (items.length === 1) {
         const item = items[0];
         if (item.type === ArtifactBrowserType.File) {

            try {
               backend.downloadArtifactV2(artifact.id, path + item.text);
            } catch (err) {
               console.error(err);
            } finally {

            }

            return;
         }
      }

      let zipRequest: CreateZipRequest | undefined;

      if ((items?.length ?? 0) > 0) {

         const filter = items.map(s => {
            const item = s as ArtifactBrowserItem;

            if (item.type === ArtifactBrowserType.NavigateUp) {
               return "";
            }

            if (item.type === ArtifactBrowserType.Directory) {
               return `${path}${item.text}/...`;
            }
            return `${path}${item.text}`;
         }).filter(f => !!f);

         zipRequest = {
            filter: filter
         }
      } else {
         if (path) {
            zipRequest = {
               filter: [path + "..."]
            }
         }
      }

      try {
         backend.downloadArtifactZipV2(artifact.id, zipRequest);
      } catch (err) {
         console.error(err);
      } finally {

      }
   }

   if (artifact?.id) {

      downloadProps.items.push(
         {
            key: 'download_zip',
            text: 'Download Zip',
            onClick: () => {
               downloadZip();
            }
         }
      )

      downloadProps.items.push(
         {
            key: 'download_ugs',
            text: 'Download with UGS',
            onClick: () => {
               downloadUGS();
            }
         }
      )
      downloadProps.items.push(
         {
            key: 'download_toolbox',
            text: 'Download with Toolbox',
            onClick: () => {
               downloadToolbox();
            }
         }
      )
   }

   return <PrimaryButton split menuProps={downloadProps} text={buttonText} styles={{ root: { minWidth: minWidth ?? 104, fontFamily: 'Horde Open Sans SemiBold !important' } }} disabled={disabled} onClick={() => {
      if (format === DownloadFormat.Zip) {
         downloadZip();
      }
      else if (format === DownloadFormat.UGS) {
         downloadUGS();
      }
      else if (format === DownloadFormat.UTOOLBOX) {
         downloadToolbox();
      }

   }} />

}