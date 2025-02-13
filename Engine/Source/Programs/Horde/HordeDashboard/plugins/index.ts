import { RouteObject } from "react-router-dom";

// Routes => components
// backend queries
// mounting points???

export enum MountType {
   TopNav
}

export type Mount = {
   type: MountType,
   context?: string;
   text: string;
   route: string;
}

export type HordePlugin = {
   id: string;
   routes: RouteObject[];
   mount?: Mount;
   enabled?: boolean;
}

const plugins: HordePlugin[] = [];

export function registerHordePlugin(plugin: HordePlugin) {
   const existing = plugins.find(p => p.id === plugin.id);
   if (existing) {
      throw `Duplicate plugin registration for id: ${plugin.id}`;
   }

   plugins.push(plugin);
}

export function enableHordePlugins(ids: string[]) {
   const set = new Set(ids);
   plugins.forEach(p => {
      p.enabled = set.has(p.id)
   });
}

export function getHordePlugins(): HordePlugin[] {
   return plugins.filter(p => !!p.enabled);
}

